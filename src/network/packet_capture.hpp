#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include "../core/data/circular_buffer.hpp"

namespace netsentry {
namespace network {

struct PacketInfo {
    std::vector<uint8_t> data;
    size_t size;
    std::string source_ip;
    std::string dest_ip;
    uint16_t source_port;
    uint16_t dest_port;
    uint8_t protocol;
    uint64_t timestamp;
};

enum class CaptureError {
    NONE,
    PERMISSION_DENIED,
    INTERFACE_NOT_FOUND,
    ALREADY_RUNNING,
    SYSTEM_ERROR
};

class PacketCapture {
public:
    using PacketHandler = std::function<void(const PacketInfo&)>;

    PacketCapture();
    ~PacketCapture();

    PacketCapture(const PacketCapture&) = delete;
    PacketCapture& operator=(const PacketCapture&) = delete;

    PacketCapture(PacketCapture&&) noexcept;
    PacketCapture& operator=(PacketCapture&&) noexcept;

    CaptureError startCapture(const std::string& interface_name);
    void stopCapture();

    void registerHandler(PacketHandler handler);

    bool isCapturing() const;

    uint64_t getPacketsCaptured() const;
    uint64_t getBytesCaptured() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;

    std::atomic<bool> is_capturing_{false};
    std::atomic<uint64_t> packets_captured_{0};
    std::atomic<uint64_t> bytes_captured_{0};

    std::thread capture_thread_;
    data::CircularBuffer<PacketInfo, 1024> packet_buffer_;

    std::vector<PacketHandler> handlers_;
    std::mutex handlers_mutex_;

    void captureThreadFunc();
    void processPacket(PacketInfo&& packet);
};

}
}
