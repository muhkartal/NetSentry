#include "rest_api.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace netsentry {
namespace api {

class RestApi::ServerImpl {
private:
    std::unordered_map<std::string, std::unordered_map<HttpMethod, RouteHandler>> routes_;
    net::io_context ioc_;
    std::vector<std::thread> threads_;
    std::atomic<bool> running_{false};

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket socket, const std::unordered_map<std::string, std::unordered_map<HttpMethod, RouteHandler>>& routes)
            : socket_(std::move(socket)), routes_(routes) {}

        void start() {
            readRequest();
        }

    private:
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        http::response<http::string_body> res_;
        const std::unordered_map<std::string, std::unordered_map<HttpMethod, RouteHandler>>& routes_;

        void readRequest() {
            auto self = shared_from_this();

            http::async_read(socket_, buffer_, req_,
                [self](beast::error_code ec, std::size_t) {
                    if (!ec) {
                        self->processRequest();
                    }
                });
        }

        void processRequest() {
            res_.version(req_.version());
            res_.keep_alive(req_.keep_alive());

            HttpMethod method;
            switch (req_.method()) {
                case http::verb::get:    method = HttpMethod::GET; break;
                case http::verb::post:   method = HttpMethod::POST; break;
                case http::verb::put:    method = HttpMethod::PUT; break;
                case http::verb::delete_: method = HttpMethod::DELETE; break;
                case http::verb::options: method = HttpMethod::OPTIONS; break;
                default:
                    res_.result(http::status::method_not_allowed);
                    res_.set(http::field::content_type, "text/plain");
                    res_.body() = "Method not allowed";
                    return writeResponse();
            }

            std::string target = req_.target().to_string();
            std::string path = target;
            std::string query;

            auto pos = target.find('?');
            if (pos != std::string::npos) {
                path = target.substr(0, pos);
                query = target.substr(pos + 1);
            }

            HttpRequest request;
            request.method = method;
            request.path = path;
            request.body = req_.body();

            for (auto const& field : req_) {
                request.headers[field.name_string().to_string()] = field.value().to_string();
            }

            if (!query.empty()) {
                parseQueryParams(query, request.query_params);
            }

            bool route_found = false;

            if (routes_.find(path) != routes_.end() && routes_.at(path).find(method) != routes_.at(path).end()) {
                auto handler = routes_.at(path).at(method);
                auto response = handler(request);

                res_.result(static_cast<http::status>(response.status_code));

                for (const auto& header : response.headers) {
                    res_.set(header.first, header.second);
                }

                if (response.headers.find("Content-Type") == response.headers.end()) {
                    res_.set(http::field::content_type, "application/json");
                }

                res_.body() = response.body;
                route_found = true;
            }

            if (!route_found) {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Not found";
            }

            writeResponse();
        }

        void parseQueryParams(const std::string& query, std::unordered_map<std::string, std::string>& params) {
            size_t start = 0;
            size_t end = query.find('&');

            while (start < query.length()) {
                std::string param = query.substr(start, end - start);
                size_t eq_pos = param.find('=');

                if (eq_pos != std::string::npos) {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    params[key] = value;
                }

                start = end + 1;
                if (end == std::string::npos) {
                    break;
                }
                end = query.find('&', start);
                if (end == std::string::npos) {
                    end = query.length();
                }
            }
        }

        void writeResponse() {
            auto self = shared_from_this();
            res_.prepare_payload();

            http::async_write(socket_, res_,
                [self](beast::error_code ec, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
        }
    };

    class Listener : public std::enable_shared_from_this<Listener> {
    public:
        Listener(
            net::io_context& ioc,
            tcp::endpoint endpoint,
            const std::unordered_map<std::string, std::unordered_map<HttpMethod, RouteHandler>>& routes)
            : ioc_(ioc), acceptor_(net::make_strand(ioc)), routes_(routes) {

            beast::error_code ec;

            acceptor_.open(endpoint.protocol(), ec);
            if (ec) {
                throw std::runtime_error("Failed to open acceptor: " + ec.message());
            }

            acceptor_.set_option(net::socket_base::reuse_address(true), ec);
            if (ec) {
                throw std::runtime_error("Failed to set reuse_address: " + ec.message());
            }

            acceptor_.bind(endpoint, ec);
            if (ec) {
                throw std::runtime_error("Failed to bind: " + ec.message());
            }

            acceptor_.listen(net::socket_base::max_listen_connections, ec);
            if (ec) {
                throw std::runtime_error("Failed to listen: " + ec.message());
            }
        }

        void run() {
            acceptor_.async_accept(net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::onAccept, shared_from_this()));
        }

    private:
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        const std::unordered_map<std::string, std::unordered_map<HttpMethod, RouteHandler>>& routes_;

        void onAccept(beast::error_code ec, tcp::socket socket) {
            if (ec) {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            } else {
                std::make_shared<Session>(std::move(socket), routes_)->start();
            }

            acceptor_.async_accept(net::make_strand(ioc_),
                beast::bind_front_handler(&Listener::onAccept, shared_from_this()));
        }
    };

public:
    ServerImpl() {}

    void addRoute(const std::string& path, HttpMethod method, RouteHandler handler) {
        routes_[path][method] = std::move(handler);
    }

    void run(uint16_t port, int num_threads) {
        try {
            auto address = net::ip::make_address("0.0.0.0");
            auto endpoint = tcp::endpoint(address, port);

            auto listener = std::make_shared<Listener>(ioc_, endpoint, routes_);
            listener->run();

            running_ = true;

            threads_.reserve(num_threads);
            for (int i = 0; i < num_threads; ++i) {
                threads_.emplace_back([this] { ioc_.run(); });
            }

            std::cout << "REST API server running on port " << port << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error starting REST API server: " << e.what() << std::endl;
            running_ = false;
        }
    }

    void stop() {
        if (running_) {
            ioc_.stop();

            for (auto& thread : threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            threads_.clear();
            running_ = false;

            std::cout << "REST API server stopped" << std::endl;
        }
    }

    bool isRunning() const {
        return running_;
    }
};

RestApi::RestApi(
    std::vector<std::unique_ptr<collectors::CollectorBase>>& collectors,
    std::unique_ptr<network::PacketAnalyzer>& packet_analyzer)
    : collectors_(collectors), packet_analyzer_(packet_analyzer) {

    server_impl_ = std::make_unique<ServerImpl>();
    setupRoutes();
}

RestApi::~RestApi() {
    stop();
}

void RestApi::start(uint16_t port) {
    if (!running_) {
        server_thread_ = std::thread([this, port] {
            int num_threads = std::thread::hardware_concurrency();
            server_impl_->run(port, num_threads > 0 ? num_threads : 2);
        });
        running_ = true;
    }
}

void RestApi::stop() {
    if (running_) {
        server_impl_->stop();

        if (server_thread_.joinable()) {
            server_thread_.join();
        }

        running_ = false;
    }
}

void RestApi::setupRoutes() {
    server_impl_->addRoute("/api/v1/metrics", HttpMethod::GET,
        [this](const HttpRequest& request) { return handleGetMetrics(request); });

    server_impl_->addRoute("/api/v1/metrics/{name}", HttpMethod::GET,
        [this](const HttpRequest& request) { return handleGetMetric(request); });

    server_impl_->addRoute("/api/v1/network/stats", HttpMethod::GET,
        [this](const HttpRequest& request) { return handleGetNetworkStats(request); });

    server_impl_->addRoute("/api/v1/network/connections", HttpMethod::GET,
        [this](const HttpRequest& request) { return handleGetConnections(request); });

    server_impl_->addRoute("/api/v1/network/hosts", HttpMethod::GET,
        [this](const HttpRequest& request) { return handleGetTopHosts(request); });

    server_impl_->addRoute("/api/v1/system/info", HttpMethod::GET,
        [this](const HttpRequest& request) { return handleGetSystemInfo(request); });
}

HttpResponse RestApi::handleGetMetrics(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";

    std::string json = "{\n  \"metrics\": [\n";
    bool first = true;

    for (const auto& collector : collectors_) {
        for (const auto& name : collector->getMetricNames()) {
            auto metric = collector->getMetric(name);
            if (metric) {
                if (!first) {
                    json += ",\n";
                }
                json += "    {\n";
                json += "      \"name\": \"" + name + "\",\n";
                json += "      \"value\": " + std::to_string(metric->getCurrentValue()) + "\n";
                json += "    }";
                first = false;
            }
        }
    }

    json += "\n  ]\n}";
    response.body = json;

    return response;
}

HttpResponse RestApi::handleGetMetric(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";

    std::string metric_name = request.path.substr(request.path.find_last_of('/') + 1);

    for (const auto& collector : collectors_) {
        auto metric = collector->getMetric(metric_name);
        if (metric) {
            response.body = "{\n";
            response.body += "  \"name\": \"" + metric_name + "\",\n";
            response.body += "  \"value\": " + std::to_string(metric->getCurrentValue()) + "\n";
            response.body += "}";
            return response;
        }
    }

    response.status_code = 404;
    response.body = "{\n  \"error\": \"Metric not found\"\n}";

    return response;
}

HttpResponse RestApi::handleGetNetworkStats(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";

    if (!packet_analyzer_) {
        response.status_code = 503;
        response.body = "{\n  \"error\": \"Network packet analyzer not available\"\n}";
        return response;
    }

    // Implementation would depend on PacketAnalyzer's interface
    response.body = "{\n";
    response.body += "  \"status\": \"Active\",\n";
    response.body += "  \"connections\": " + std::to_string(packet_analyzer_->getTopConnections(1000).size()) + "\n";
    response.body += "}";

    return response;
}

HttpResponse RestApi::handleGetConnections(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";

    if (!packet_analyzer_) {
        response.status_code = 503;
        response.body = "{\n  \"error\": \"Network packet analyzer not available\"\n}";
        return response;
    }

    size_t limit = 10;
    auto it = request.query_params.find("limit");
    if (it != request.query_params.end()) {
        try {
            limit = std::stoul(it->second);
        } catch (...) {
            limit = 10;
        }
    }

    auto connections = packet_analyzer_->getTopConnections(limit);

    std::string json = "{\n  \"connections\": [\n";
    bool first = true;

    for (const auto& conn : connections) {
        const auto& key = conn.first;
        const auto& stats = conn.second;

        if (!first) {
            json += ",\n";
        }

        json += "    {\n";
        json += "      \"source\": \"" + key.source_ip + ":" + std::to_string(key.source_port) + "\",\n";
        json += "      \"destination\": \"" + key.dest_ip + ":" + std::to_string(key.dest_port) + "\",\n";
        json += "      \"protocol\": " + std::to_string(key.protocol) + ",\n";
        json += "      \"bytes_sent\": " + std::to_string(stats.bytes_sent) + ",\n";
        json += "      \"bytes_received\": " + std::to_string(stats.bytes_received) + ",\n";
        json += "      \"packets_sent\": " + std::to_string(stats.packets_sent) + ",\n";
        json += "      \"packets_received\": " + std::to_string(stats.packets_received) + "\n";
        json += "    }";

        first = false;
    }

    json += "\n  ]\n}";
    response.body = json;

    return response;
}

HttpResponse RestApi::handleGetTopHosts(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";

    if (!packet_analyzer_) {
        response.status_code = 503;
        response.body = "{\n  \"error\": \"Network packet analyzer not available\"\n}";
        return response;
    }

    size_t limit = 10;
    auto it = request.query_params.find("limit");
    if (it != request.query_params.end()) {
        try {
            limit = std::stoul(it->second);
        } catch (...) {
            limit = 10;
        }
    }

    auto host_stats = packet_analyzer_->getHostTrafficStats();

    std::vector<std::pair<std::string, uint64_t>> sorted_hosts;
    for (const auto& entry : host_stats) {
        sorted_hosts.emplace_back(entry);
    }

    std::sort(sorted_hosts.begin(), sorted_hosts.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::string json = "{\n  \"hosts\": [\n";
    bool first = true;

    for (size_t i = 0; i < std::min(limit, sorted_hosts.size()); ++i) {
        if (!first) {
            json += ",\n";
        }

        json += "    {\n";
        json += "      \"ip\": \"" + sorted_hosts[i].first + "\",\n";
        json += "      \"bytes\": " + std::to_string(sorted_hosts[i].second) + "\n";
        json += "    }";

        first = false;
    }

    json += "\n  ]\n}";
    response.body = json;

    return response;
}

HttpResponse RestApi::handleGetSystemInfo(const HttpRequest& request) {
    HttpResponse response;
    response.headers["Content-Type"] = "application/json";

    response.body = "{\n";
    response.body += "  \"hostname\": \"" + getSystemHostname() + "\",\n";
    response.body += "  \"platform\": \"" + getSystemPlatform() + "\",\n";
    response.body += "  \"num_cpus\": " + std::to_string(std::thread::hardware_concurrency()) + ",\n";
    response.body += "  \"uptime\": " + std::to_string(getSystemUptime()) + "\n";
    response.body += "}";

    return response;
}

// Helper system info functions (platform-specific)
std::string getSystemHostname() {
    char hostname[1024];
    hostname[1023] = '\0';

    if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
        return "unknown";
    }

    return hostname;
}

std::string getSystemPlatform() {
#ifdef _WIN32
    return "Windows";
#elif __APPLE__
    return "macOS";
#elif __linux__
    return "Linux";
#else
    return "Unknown";
#endif
}

uint64_t getSystemUptime() {
    std::ifstream uptime_file("/proc/uptime");
    double uptime = 0;

    if (uptime_file.is_open()) {
        uptime_file >> uptime;
    }

    return static_cast<uint64_t>(uptime);
}

}
}
