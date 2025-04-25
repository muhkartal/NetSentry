#include "memory_collector.hpp"
#include <fstream>
#include <string>
#include <sstream>

namespace netsentry {
namespace collectors {

MemoryCollector::MemoryCollector(std::chrono::seconds interval)
    : CollectorBase(std::chrono::milliseconds(interval)) {

    memory_total_ = std::make_shared<metrics::GaugeMetric>("memory.total");
    memory_used_ = std::make_shared<metrics::GaugeMetric>("memory.used");
    memory_free_ = std::make_shared<metrics::GaugeMetric>("memory.free");
    memory_usage_percent_ = std::make_shared<metrics::GaugeMetric>("memory.usage_percent");
    swap_total_ = std::make_shared<metrics::GaugeMetric>("memory.swap_total");
    swap_used_ = std::make_shared<metrics::GaugeMetric>("memory.swap_used");
    swap_usage_percent_ = std::make_shared<metrics::GaugeMetric>("memory.swap_usage_percent");

    registerMetric(memory_total_);
    registerMetric(memory_used_);
    registerMetric(memory_free_);
    registerMetric(memory_usage_percent_);
    registerMetric(swap_total_);
    registerMetric(swap_used_);
    registerMetric(swap_usage_percent_);
}

void MemoryCollector::collect() {
    auto stats = readMemoryStats();

    memory_total_->update(static_cast<double>(stats.total) / 1024.0);
    memory_used_->update(static_cast<double>(stats.used) / 1024.0);
    memory_free_->update(static_cast<double>(stats.free) / 1024.0);

    if (stats.total > 0) {
        double usage_percent = 100.0 * static_cast<double>(stats.used) / stats.total;
        memory_usage_percent_->update(usage_percent);
    }

    swap_total_->update(static_cast<double>(stats.swap_total) / 1024.0);
    swap_used_->update(static_cast<double>(stats.swap_used) / 1024.0);

    if (stats.swap_total > 0) {
        double swap_usage_percent = 100.0 * static_cast<double>(stats.swap_used) / stats.swap_total;
        swap_usage_percent_->update(swap_usage_percent);
    }
}

MemoryStats MemoryCollector::readMemoryStats() {
    MemoryStats stats{};

#ifdef _WIN32
    // Windows-specific implementation
    // For simplicity, return dummy values
    stats.total = 16384 * 1024;
    stats.free = 8192 * 1024;
    stats.available = 8192 * 1024;
    stats.used = stats.total - stats.free;
    stats.buffers = 0;
    stats.cached = 0;
    stats.swap_total = 4096 * 1024;
    stats.swap_free = 4096 * 1024;
    stats.swap_used = 0;
#else
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        std::string unit;

        iss >> key >> value >> unit;

        if (key == "MemTotal:") {
            stats.total = value;
        } else if (key == "MemFree:") {
            stats.free = value;
        } else if (key == "MemAvailable:") {
            stats.available = value;
        } else if (key == "Buffers:") {
            stats.buffers = value;
        } else if (key == "Cached:") {
            stats.cached = value;
        } else if (key == "SwapTotal:") {
            stats.swap_total = value;
        } else if (key == "SwapFree:") {
            stats.swap_free = value;
        }
    }

    stats.used = stats.total - stats.free - stats.buffers - stats.cached;
    stats.swap_used = stats.swap_total - stats.swap_free;
#endif

    return stats;
}

}
}
