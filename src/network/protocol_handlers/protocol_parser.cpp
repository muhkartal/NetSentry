#include "protocol_parser.hpp"
#include <algorithm>
#include <string>
#include <cstring>
#include <array>

namespace netsentry {
namespace network {

std::unique_ptr<ProtocolData> HttpParser::parse(const PacketInfo& packet) {
    if (!isHttpPacket(packet)) {
        return nullptr;
    }

    if (packet.source_port == 80 || packet.source_port == 8080) {
        return parseHttpResponse(packet.data);
    } else if (packet.dest_port == 80 || packet.dest_port == 8080) {
        return parseHttpRequest(packet.data);
    }

    return nullptr;
}

bool HttpParser::isHttpPacket(const PacketInfo& packet) const {
    if (packet.protocol != IPPROTO_TCP) {
        return false;
    }

    if (packet.data.size() < 16) {
        return false;
    }

    static const std::array<std::string, 9> http_methods = {
        "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "CONNECT", "TRACE"
    };

    std::string start(packet.data.begin(), packet.data.begin() + std::min<size_t>(8, packet.data.size()));

    for (const auto& method : http_methods) {
        if (start.find(method) == 0 && start.find(" ") == method.length()) {
            return true;
        }
    }

    return start.find("HTTP/") == 0;
}

std::unique_ptr<HttpData> HttpParser::parseHttpRequest(const std::vector<uint8_t>& data) {
    auto http_data = std::make_unique<HttpData>();
    http_data->type = ProtocolType::HTTP;
    http_data->is_request = true;

    std::string raw_data(data.begin(), data.end());
    size_t end_of_headers = raw_data.find("\r\n\r\n");

    if (end_of_headers == std::string::npos) {
        end_of_headers = raw_data.length();
    }

    std::string headers = raw_data.substr(0, end_of_headers);

    size_t line_end = headers.find("\r\n");
    if (line_end == std::string::npos) {
        return http_data;
    }

    std::string request_line = headers.substr(0, line_end);

    size_t method_end = request_line.find(" ");
    if (method_end == std::string::npos) {
        return http_data;
    }

    http_data->method = request_line.substr(0, method_end);

    size_t uri_end = request_line.find(" ", method_end + 1);
    if (uri_end == std::string::npos) {
        return http_data;
    }

    http_data->uri = request_line.substr(method_end + 1, uri_end - method_end - 1);
    http_data->http_version = request_line.substr(uri_end + 1);

    size_t header_start = line_end + 2;
    while (header_start < headers.length()) {
        size_t header_end = headers.find("\r\n", header_start);
        if (header_end == std::string::npos) {
            header_end = headers.length();
        }

        std::string header_line = headers.substr(header_start, header_end - header_start);
        size_t colon_pos = header_line.find(":");

        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 1);

            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value.erase(0, 1);
            }

            http_data->headers[key] = value;
        }

        header_start = header_end + 2;
    }

    return http_data;
}

std::unique_ptr<HttpData> HttpParser::parseHttpResponse(const std::vector<uint8_t>& data) {
    auto http_data = std::make_unique<HttpData>();
    http_data->type = ProtocolType::HTTP;
    http_data->is_request = false;

    std::string raw_data(data.begin(), data.end());
    size_t end_of_headers = raw_data.find("\r\n\r\n");

    if (end_of_headers == std::string::npos) {
        end_of_headers = raw_data.length();
    }

    std::string headers = raw_data.substr(0, end_of_headers);

    size_t line_end = headers.find("\r\n");
    if (line_end == std::string::npos) {
        return http_data;
    }

    std::string status_line = headers.substr(0, line_end);

    size_t http_version_end = status_line.find(" ");
    if (http_version_end == std::string::npos) {
        return http_data;
    }

    http_data->http_version = status_line.substr(0, http_version_end);

    size_t status_code_end = status_line.find(" ", http_version_end + 1);
    if (status_code_end == std::string::npos) {
        status_code_end = status_line.length();
    }

    std::string status_code_str = status_line.substr(http_version_end + 1, status_code_end - http_version_end - 1);
    try {
        http_data->status_code = std::stoi(status_code_str);
    } catch (...) {
        http_data->status_code = 0;
    }

    size_t header_start = line_end + 2;
    while (header_start < headers.length()) {
        size_t header_end = headers.find("\r\n", header_start);
        if (header_end == std::string::npos) {
            header_end = headers.length();
        }

        std::string header_line = headers.substr(header_start, header_end - header_start);
        size_t colon_pos = header_line.find(":");

        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 1);

            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value.erase(0, 1);
            }

            http_data->headers[key] = value;
        }

        header_start = header_end + 2;
    }

    return http_data;
}

std::unique_ptr<ProtocolData> DnsParser::parse(const PacketInfo& packet) {
    if (!isDnsPacket(packet)) {
        return nullptr;
    }

    return parseDnsPacket(packet.data);
}

bool DnsParser::isDnsPacket(const PacketInfo& packet) const {
    return (packet.protocol == IPPROTO_UDP &&
            (packet.source_port == 53 || packet.dest_port == 53)) ||
           (packet.protocol == IPPROTO_TCP &&
            (packet.source_port == 53 || packet.dest_port == 53));
}

std::unique_ptr<DnsData> DnsParser::parseDnsPacket(const std::vector<uint8_t>& data) {
    auto dns_data = std::make_unique<DnsData>();
    dns_data->type = ProtocolType::DNS;

    if (data.size() < 12) {
        return dns_data;
    }

    uint16_t transaction_id = (data[0] << 8) | data[1];
    uint16_t flags = (data[2] << 8) | data[3];
    uint16_t questions = (data[4] << 8) | data[5];
    uint16_t answers = (data[6] << 8) | data[7];

    dns_data->transaction_id = transaction_id;
    dns_data->is_query = !(flags & 0x8000);

    return dns_data;
}

std::unique_ptr<ProtocolData> TlsParser::parse(const PacketInfo& packet) {
    if (!isTlsPacket(packet)) {
        return nullptr;
    }

    return parseTlsPacket(packet.data);
}

bool TlsParser::isTlsPacket(const PacketInfo& packet) const {
    if (packet.protocol != IPPROTO_TCP) {
        return false;
    }

    if (packet.data.size() < 5) {
        return false;
    }

    uint8_t content_type = packet.data[0];
    uint16_t version = (packet.data[1] << 8) | packet.data[2];

    return (content_type >= 20 && content_type <= 23) &&
           ((version >= 0x0300 && version <= 0x0304) || version == 0x0100);
}

std::unique_ptr<TlsData> TlsParser::parseTlsPacket(const std::vector<uint8_t>& data) {
    auto tls_data = std::make_unique<TlsData>();
    tls_data->type = ProtocolType::TLS;

    if (data.size() < 5) {
        return tls_data;
    }

    tls_data->content_type = data[0];
    tls_data->version = (data[1] << 8) | data[2];

    tls_data->is_handshake = (tls_data->content_type == 22);

    if (tls_data->is_handshake && data.size() >= 6) {
        uint8_t handshake_type = data[5];
        tls_data->is_client_hello = (handshake_type == 1);
        tls_data->is_server_hello = (handshake_type == 2);

        if (tls_data->is_client_hello && data.size() > 43) {
            uint16_t session_id_length = data[43];
            size_t extensions_offset = 44 + session_id_length;

            if (data.size() > extensions_offset + 2) {
                uint16_t cipher_suites_length = (data[extensions_offset] << 8) | data[extensions_offset + 1];
                extensions_offset += 2 + cipher_suites_length;

                if (data.size() > extensions_offset + 1) {
                    uint8_t compression_methods_length = data[extensions_offset];
                    extensions_offset += 1 + compression_methods_length;

                    if (data.size() > extensions_offset + 2) {
                        uint16_t extensions_length = (data[extensions_offset] << 8) | data[extensions_offset + 1];
                        extensions_offset += 2;

                        tls_data->server_name = extractServerName(data, extensions_offset, extensions_length);
                    }
                }
            }
        }
    }

    return tls_data;
}

std::optional<std::string> TlsParser::extractServerName(const std::vector<uint8_t>& data, size_t offset, size_t length) {
    size_t end_offset = offset + length;
    size_t pos = offset;

    while (pos + 4 <= end_offset) {
        uint16_t extension_type = (data[pos] << 8) | data[pos + 1];
        uint16_t extension_length = (data[pos + 2] << 8) | data[pos + 3];
        pos += 4;

        if (extension_type == 0 && pos + extension_length <= end_offset && extension_length > 2) {
            uint16_t list_length = (data[pos] << 8) | data[pos + 1];
            pos += 2;

            if (list_length > 3 && pos + list_length <= end_offset) {
                uint8_t name_type = data[pos];
                uint16_t name_length = (data[pos + 1] << 8) | data[pos + 2];
                pos += 3;

                if (name_type == 0 && pos + name_length <= end_offset) {
                    return std::string(data.begin() + pos, data.begin() + pos + name_length);
                }
            }
        }

        pos += extension_length;
    }

    return std::nullopt;
}

std::vector<std::unique_ptr<ProtocolParser>> ProtocolParserFactory::createAllParsers() {
    std::vector<std::unique_ptr<ProtocolParser>> parsers;

    parsers.push_back(std::make_unique<HttpParser>());
    parsers.push_back(std::make_unique<DnsParser>());
    parsers.push_back(std::make_unique<TlsParser>());

    return parsers;
}

std::unique_ptr<ProtocolParser> ProtocolParserFactory::createParser(ProtocolType type) {
    switch (type) {
        case ProtocolType::HTTP:
            return std::make_unique<HttpParser>();
        case ProtocolType::DNS:
            return std::make_unique<DnsParser>();
        case ProtocolType::TLS:
            return std::make_unique<TlsParser>();
        default:
            return nullptr;
    }
}

}
}
