#include "packet_capture.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <pcap/pcap.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#endif

namespace netsentry {
namespace network {

class PacketCapture::Impl {
public:
    Impl() : pcap_handle_(nullptr) {}
    ~Impl() {
        if (pcap_handle_) {
            pcap_close(pcap_handle_);
        }
    }

    CaptureError openInterface(const std::string& interface_name) {
        char errbuf[PCAP_ERRBUF_SIZE];

        pcap_handle_ = pcap_open_live(interface_name.c_str(), 65536, 1, 1000, errbuf);
        if (!pcap_handle_) {
            if (strstr(errbuf, "permission")) {
                return CaptureError::PERMISSION_DENIED;
            } else if (strstr(errbuf, "exist") || strstr(errbuf, "found")) {
                return CaptureError::INTERFACE_NOT_FOUND;
            } else {
                return CaptureError::SYSTEM_ERROR;
            }
        }

        return CaptureError::NONE;
    }

    PacketInfo capturePacket() {
        struct pcap_pkthdr* header;
        const u_char* packet_data;
        int result = pcap_next_ex(pcap_handle_, &header, &packet_data);

        PacketInfo packet;
        if (result <= 0) {
            return packet;
        }

        packet.timestamp = static_cast<uint64_t>(header->ts.tv_sec) * 1000000 + header->ts.tv_usec;
        packet.size = header->len;
        packet.data.assign(packet_data, packet_data + header->caplen);

        parsePacket(packet, packet_data, header->caplen);

        return packet;
    }

    void close() {
        if (pcap_handle_) {
            pcap_close(pcap_handle_);
            pcap_handle_ = nullptr;
        }
    }

private:
    pcap_t* pcap_handle_;

    void parsePacket(PacketInfo& packet, const u_char* packet_data, size_t len) {
        if (len < 14) {
            return;
        }

        const u_char* ip_header = packet_data + 14;
        size_t ip_header_len = (ip_header[0] & 0x0F) * 4;

        if (len < 14 + ip_header_len) {
            return;
        }

        packet.protocol = ip_header[9];

        struct in_addr src_addr, dst_addr;
        memcpy(&src_addr, ip_header + 12, 4);
        memcpy(&dst_addr, ip_header + 16, 4);

        packet.source_ip = inet_ntoa(src_addr);
        char dst_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &dst_addr, dst_ip, INET_ADDRSTRLEN);
        packet.dest_ip = dst_ip;

        const u_char* transport_header = ip_header + ip_header_len;

        if (packet.protocol == IPPROTO_TCP && len >= 14 + ip_header_len + 20) {
            packet.source_port = ntohs(*reinterpret_cast<const uint16_t*>(transport_header));
            packet.dest_port = ntohs(*reinterpret_cast<const uint16_t*>(transport_header + 2));
        } else if (packet.protocol == IPPROTO_UDP && len >= 14 + ip_header_len + 8) {
            packet.source_port = ntohs(*reinterpret_cast<const uint16_t*>(transport_header));
            packet.dest_port = ntohs(*reinterpret_cast<const uint16_t*>(transport_header + 2));
        } else {
            packet.source_port = 0;
            packet.dest_port = 0;
        }
    }
};

PacketCapture::PacketCapture() : pimpl_(std::make_unique<Impl>()) {}

PacketCapture::~PacketCapture() {
    stopCapture();
}

PacketCapture::PacketCapture(PacketCapture&& other) noexcept
    : pimpl_(std::move(other.pimpl_)),
      is_capturing_(other.is_capturing_.load()),
      packets_captured_(other.packets_captured_.load()),
      bytes_captured_(other.bytes_captured_.load()),
      handlers_(std::move(other.handlers_)) {

    other.is_capturing_ = false;
    other.packets_captured_ = 0;
    other.bytes_captured_ = 0;
}

PacketCapture& PacketCapture::operator=(PacketCapture&& other) noexcept {
    if (this != &other) {
        stopCapture();

        pimpl_ = std::move(other.pimpl_);
        is_capturing_ = other.is_capturing_.load();
        packets_captured_ = other.packets_captured_.load();
        bytes_captured_ = other.bytes_captured_.load();

        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_ = std::move(other.handlers_);

        other.is_capturing_ = false;
        other.packets_captured_ = 0;
        other.bytes_captured_ = 0;
    }
    return *this;
}

CaptureError PacketCapture::startCapture(const std::string& interface_name) {
    if (is_capturing_) {
        return CaptureError::ALREADY_RUNNING;
    }

    auto error = pimpl_->openInterface(interface_name);
    if (error != CaptureError::NONE) {
        return error;
    }

    is_capturing_ = true;
    packets_captured_ = 0;
    bytes_captured_ = 0;

    capture_thread_ = std::thread(&PacketCapture::captureThreadFunc, this);

    return CaptureError::NONE;
}

void PacketCapture::stopCapture() {
    if (!is_capturing_) {
        return;
    }

    is_capturing_ = false;

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

    pimpl_->close();
}

void PacketCapture::registerHandler(PacketHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_.push_back(std::move(handler));
}

bool PacketCapture::isCapturing() const {
    return is_capturing_;
}

uint64_t PacketCapture::getPacketsCaptured() const {
    return packets_captured_;
}

uint64_t PacketCapture::getBytesCaptured() const {
    return bytes_captured_;
}

void PacketCapture::captureThreadFunc() {
    while (is_capturing_) {
        auto packet = pimpl_->capturePacket();

        if (packet.size > 0) {
            processPacket(std::move(packet));
        }
    }
}

void PacketCapture::processPacket(PacketInfo&& packet) {
    packets_captured_++;
    bytes_captured_ += packet.size;

    std::lock_guard<std::mutex> lock(handlers_mutex_);
    for (const auto& handler : handlers_) {
        handler(packet);
    }

    packet_buffer_.push(std::move(packet));
}

}
}
