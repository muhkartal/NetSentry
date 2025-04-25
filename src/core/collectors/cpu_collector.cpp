#include "cpu_collector.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace netsentry {
namespace collectors {

CpuCollector::CpuCollector(std::chrono::seconds interval)
    : CollectorBase(std::chrono::milliseconds(interval)) {

    cpu_usage_ = std::make_shared<metrics::GaugeMetric>("cpu.usage");
    registerMetric(cpu_usage_);

    prev_stats_ = readCpuStats();

    core_usage_.resize(prev_stats_.size() - 1);
    for (size_t i = 0; i < core_usage_.size(); ++i) {
        core_usage_[i] = std::make_shared<metrics::GaugeMetric>("cpu.core." + std::to_string(i) + ".usage");
        registerMetric(core_usage_[i]);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void CpuCollector::collect() {
    auto curr_stats = readCpuStats();

    if (curr_stats.size() != prev_stats_.size()) {
        prev_stats_ = curr_stats;
        return;
    }

    double total_usage = calculateCpuUsage(prev_stats_[0], curr_stats[0]);
    cpu_usage_->update(total_usage);

    for (size_t i = 0; i < core_usage_.size(); ++i) {
        double core_usage = calculateCpuUsage(prev_stats_[i + 1], curr_stats[i + 1]);
        core_usage_[i]->update(core_usage);
    }

    prev_stats_ = std::move(curr_stats);
}

std::vector<CpuStats> CpuCollector::readCpuStats() {
    std::vector<CpuStats> stats;

#ifdef _WIN32
    // Windows-specific implementation would go here
    // For simplicity, we'll just return a default value
    stats.push_back({10, 0, 10, 80, 0, 0, 0, 0, 0, 0});
#else
    std::ifstream proc_stat("/proc/stat");
    std::string line;

    while (std::getline(proc_stat, line)) {
        if (line.substr(0, 3) != "cpu") {
            break;
        }

        std::istringstream iss(line);
        std::string cpu_label;
        CpuStats cpu_stats{};

        iss >> cpu_label;
        iss >> cpu_stats.user >> cpu_stats.nice >> cpu_stats.system >> cpu_stats.idle
            >> cpu_stats.iowait >> cpu_stats.irq >> cpu_stats.softirq >> cpu_stats.steal
            >> cpu_stats.guest >> cpu_stats.guest_nice;

        stats.push_back(cpu_stats);
    }
#endif

    if (stats.empty()) {
        stats.push_back({});
    }

    return stats;
}

double CpuCollector::calculateCpuUsage(const CpuStats& prev, const CpuStats& curr) {
    uint64_t prev_total = prev.total();
    uint64_t curr_total = curr.total();

    uint64_t total_delta = curr_total - prev_total;
    if (total_delta == 0) {
        return 0.0;
    }

    uint64_t idle_delta = curr.idle_total() - prev.idle_total();
    double usage_percent = 100.0 * (1.0 - static_cast<double>(idle_delta) / total_delta);

    return usage_percent;
}

}
}
