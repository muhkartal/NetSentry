#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <unordered_map>

#include "../core/collectors/collector_base.hpp"
#include "../network/packet_analyzer.hpp"

namespace netsentry {
namespace web {

class Dashboard {
public:
    Dashboard(
        std::vector<std::unique_ptr<collectors::CollectorBase>>& collectors,
        std::unique_ptr<network::PacketAnalyzer>& packet_analyzer);

    ~Dashboard();

    void start(uint16_t port = 9090);
    void stop();

    bool isRunning() const { return running_; }

private:
    class ServerImpl;
    std::unique_ptr<ServerImpl> server_impl_;

    std::vector<std::unique_ptr<collectors::CollectorBase>>& collectors_;
    std::unique_ptr<network::PacketAnalyzer>& packet_analyzer_;

    std::thread server_thread_;
    std::atomic<bool> running_{false};

    std::string loadDashboardHtml();
    std::string loadDashboardCss();
    std::string loadDashboardJs();

    std::string getMetricsJson();
    std::string getNetworkStatsJson();
    std::string getConnectionsJson();
    std::string getSystemInfoJson();

    // Embed web assets during compilation
    static const std::string DASHBOARD_HTML;
    static const std::string DASHBOARD_CSS;
    static const std::string DASHBOARD_JS;
};

}
}
