#pragma once

#include <unordered_map>
#include <string>
#include <chrono>
#include <memory>
#include <vector>
#include <optional>
#include <mutex>
#include "packet_capture.hpp"
#include "../core/data/circular_buffer.hpp"
#include "protocol_handlers/protocol_parser.hpp"

namespace netsentry {
namespace network {

struct ConnectionKey {
    std::string source_ip;
    std::string dest_ip;
    uint16_t source_port;
    uint16_t dest_port;
    uint8_t protocol;

    bool operator==(const ConnectionKey& other) const {
        return source_ip == other.source_ip &&
               dest_ip == other.dest_ip &&
               source_port == other.source_port &&
               dest_port == other.dest_port &&
               protocol == other.protocol;
    }
};

struct ConnectionKeyHash {
    size_t operator()(const ConnectionKey& key) const {
        size_t h1 = std::hash<std::string>{}(key.source_ip);
        size_t h2 = std::hash<std::string>{}(key.dest_ip);
        size_t h3 = std::hash<uint16_t>{}(key.source_port);
        size_t h4 = std::hash<uint16_t>{}(key.dest_port);
        size_t h5 = std::hash<uint8_t>{}(key.protocol);

        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
    }
};

struct ConnectionStats {
    uint64_t packets_sent{0};
    uint64_t packets_received{0};
    uint64_t bytes_sent{0};
    uint64_t bytes_received{0};
    uint64_t first_seen{0};
    uint64_t last_seen{0};
    std::optional<ProtocolType> protocol_type;
    std::shared_ptr<ProtocolData> protocol_data;
};

class PacketAnalyzer {
public:
    PacketAnalyzer();

    void processPacket(const PacketInfo& packet);

    std::vector<std::pair<ConnectionKey, ConnectionStats>> getTopConnections(size_t limit) const;

    std::unordered_map<std::string, uint64_t> getHostTrafficStats() const;

    std::optional<ConnectionStats> getConnectionStats(const ConnectionKey& key) const;

    void reset();

private:
    std::unordered_map<ConnectionKey, ConnectionStats, ConnectionKeyHash> connections_;
    std::unordered_map<std::string, uint64_t> host_traffic_stats_;
    data::CircularBuffer<PacketInfo, 1000> recent_packets_;
    std::vector<std::unique_ptr<ProtocolParser>> protocol_parsers_;
    mutable std::mutex mutex_;

    void analyzeProtocol(const PacketInfo& packet, ConnectionStats& stats);
    ConnectionKey createConnectionKey(const PacketInfo& packet, bool normalize = true);

    static bool compareConnectionsByTraffic(
        const std::pair<ConnectionKey, ConnectionStats>& a,
        const std::pair<ConnectionKey, ConnectionStats>& b);
};

}
}
