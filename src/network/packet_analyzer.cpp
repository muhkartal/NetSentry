#include "packet_analyzer.hpp"
#include <algorithm>
#include <utility>

namespace netsentry {
namespace network {

PacketAnalyzer::PacketAnalyzer() {
    protocol_parsers_ = ProtocolParserFactory::createAllParsers();
}

void PacketAnalyzer::processPacket(const PacketInfo& packet) {
    std::lock_guard<std::mutex> lock(mutex_);

    recent_packets_.push(packet);

    ConnectionKey key = createConnectionKey(packet);

    auto it = connections_.find(key);
    if (it == connections_.end()) {
        ConnectionStats stats;
        stats.first_seen = packet.timestamp;
        stats.last_seen = packet.timestamp;

        if (packet.source_ip == key.source_ip && packet.source_port == key.source_port) {
            stats.packets_sent = 1;
            stats.packets_received = 0;
            stats.bytes_sent = packet.size;
            stats.bytes_received = 0;
        } else {
            stats.packets_sent = 0;
            stats.packets_received = 1;
            stats.bytes_sent = 0;
            stats.bytes_received = packet.size;
        }

        analyzeProtocol(packet, stats);

        connections_[key] = stats;
    } else {
        auto& stats = it->second;
        stats.last_seen = packet.timestamp;

        if (packet.source_ip == key.source_ip && packet.source_port == key.source_port) {
            stats.packets_sent++;
            stats.bytes_sent += packet.size;
        } else {
            stats.packets_received++;
            stats.bytes_received += packet.size;
        }

        if (!stats.protocol_type.has_value()) {
            analyzeProtocol(packet, stats);
        }
    }

    host_traffic_stats_[packet.source_ip] += packet.size;
    host_traffic_stats_[packet.dest_ip] += packet.size;
}

std::vector<std::pair<ConnectionKey, ConnectionStats>> PacketAnalyzer::getTopConnections(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<ConnectionKey, ConnectionStats>> result;
    result.reserve(connections_.size());

    for (const auto& entry : connections_) {
        result.emplace_back(entry);
    }

    std::sort(result.begin(), result.end(), compareConnectionsByTraffic);

    if (result.size() > limit) {
        result.resize(limit);
    }

    return result;
}

std::unordered_map<std::string, uint64_t> PacketAnalyzer::getHostTrafficStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return host_traffic_stats_;
}

std::optional<ConnectionStats> PacketAnalyzer::getConnectionStats(const ConnectionKey& key) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(key);
    if (it == connections_.end()) {
        return std::nullopt;
    }

    return it->second;
}

void PacketAnalyzer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    connections_.clear();
    host_traffic_stats_.clear();
}

void PacketAnalyzer::analyzeProtocol(const PacketInfo& packet, ConnectionStats& stats) {
    for (const auto& parser : protocol_parsers_) {
        auto protocol_data = parser->parse(packet);
        if (protocol_data) {
            stats.protocol_type = protocol_data->type;
            stats.protocol_data = std::move(protocol_data);
            break;
        }
    }
}

ConnectionKey PacketAnalyzer::createConnectionKey(const PacketInfo& packet, bool normalize) {
    ConnectionKey key;

    if (normalize &&
        ((packet.source_ip > packet.dest_ip) ||
         (packet.source_ip == packet.dest_ip && packet.source_port > packet.dest_port))) {
        key.source_ip = packet.dest_ip;
        key.dest_ip = packet.source_ip;
        key.source_port = packet.dest_port;
        key.dest_port = packet.source_port;
    } else {
        key.source_ip = packet.source_ip;
        key.dest_ip = packet.dest_ip;
        key.source_port = packet.source_port;
        key.dest_port = packet.dest_port;
    }

    key.protocol = packet.protocol;

    return key;
}

bool PacketAnalyzer::compareConnectionsByTraffic(
    const std::pair<ConnectionKey, ConnectionStats>& a,
    const std::pair<ConnectionKey, ConnectionStats>& b) {

    uint64_t a_traffic = a.second.bytes_sent + a.second.bytes_received;
    uint64_t b_traffic = b.second.bytes_sent + b.second.bytes_received;

    return a_traffic > b_traffic;
}

}
}
