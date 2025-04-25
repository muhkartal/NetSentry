#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "../core/metrics/system_metrics.hpp"
#include "../core/collectors/collector_base.hpp"
#include "../network/packet_analyzer.hpp"

namespace netsentry {
namespace api {

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    OPTIONS
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query_params;
    std::string body;
};

struct HttpResponse {
    int status_code{200};
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class RestApi {
public:
    RestApi(
        std::vector<std::unique_ptr<collectors::CollectorBase>>& collectors,
        std::unique_ptr<network::PacketAnalyzer>& packet_analyzer);

    ~RestApi();

    void start(uint16_t port = 8080);
    void stop();

    bool isRunning() const { return running_; }

private:
    class ServerImpl;
    std::unique_ptr<ServerImpl> server_impl_;

    std::vector<std::unique_ptr<collectors::CollectorBase>>& collectors_;
    std::unique_ptr<network::PacketAnalyzer>& packet_analyzer_;

    std::thread server_thread_;
    std::atomic<bool> running_{false};

    void setupRoutes();

    HttpResponse handleGetMetrics(const HttpRequest& request);
    HttpResponse handleGetMetric(const HttpRequest& request);
    HttpResponse handleGetNetworkStats(const HttpRequest& request);
    HttpResponse handleGetConnections(const HttpRequest& request);
    HttpResponse handleGetTopHosts(const HttpRequest& request);
    HttpResponse handleGetSystemInfo(const HttpRequest& request);
};

}
}
