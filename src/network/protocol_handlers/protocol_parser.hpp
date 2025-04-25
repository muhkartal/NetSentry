#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include "../packet_capture.hpp"

namespace netsentry {
namespace network {

enum class ProtocolType {
    UNKNOWN,
    TCP,
    UDP,
    ICMP,
    HTTP,
    DNS,
    TLS,
    SMTP
};

struct ProtocolData {
    ProtocolType type{ProtocolType::UNKNOWN};
    virtual ~ProtocolData() = default;
};

struct HttpData : public ProtocolData {
    std::string method;
    std::string uri;
    std::string http_version;
    std::unordered_map<std::string, std::string> headers;
    bool is_request{true};
    int status_code{0};
};

struct DnsData : public ProtocolData {
    uint16_t transaction_id{0};
    bool is_query{true};
    std::vector<std::string> questions;
    std::vector<std::string> answers;
};

struct TlsData : public ProtocolData {
    uint8_t content_type{0};
    uint16_t version{0};
    bool is_handshake{false};
    bool is_client_hello{false};
    bool is_server_hello{false};
    std::optional<std::string> server_name;
};

class ProtocolParser {
public:
    virtual ~ProtocolParser() = default;
    virtual ProtocolType getProtocolType() const = 0;
    virtual std::unique_ptr<ProtocolData> parse(const PacketInfo& packet) = 0;
};

class HttpParser : public ProtocolParser {
public:
    ProtocolType getProtocolType() const override { return ProtocolType::HTTP; }
    std::unique_ptr<ProtocolData> parse(const PacketInfo& packet) override;

private:
    bool isHttpPacket(const PacketInfo& packet) const;
    std::unique_ptr<HttpData> parseHttpRequest(const std::vector<uint8_t>& data);
    std::unique_ptr<HttpData> parseHttpResponse(const std::vector<uint8_t>& data);
};

class DnsParser : public ProtocolParser {
public:
    ProtocolType getProtocolType() const override { return ProtocolType::DNS; }
    std::unique_ptr<ProtocolData> parse(const PacketInfo& packet) override;

private:
    bool isDnsPacket(const PacketInfo& packet) const;
    std::unique_ptr<DnsData> parseDnsPacket(const std::vector<uint8_t>& data);
};

class TlsParser : public ProtocolParser {
public:
    ProtocolType getProtocolType() const override { return ProtocolType::TLS; }
    std::unique_ptr<ProtocolData> parse(const PacketInfo& packet) override;

private:
    bool isTlsPacket(const PacketInfo& packet) const;
    std::unique_ptr<TlsData> parseTlsPacket(const std::vector<uint8_t>& data);
    std::optional<std::string> extractServerName(const std::vector<uint8_t>& data, size_t offset, size_t length);
};

class ProtocolParserFactory {
public:
    static std::vector<std::unique_ptr<ProtocolParser>> createAllParsers();
    static std::unique_ptr<ProtocolParser> createParser(ProtocolType type);
};

}
}
