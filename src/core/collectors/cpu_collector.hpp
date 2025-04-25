#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include "collector_base.hpp"

namespace netsentry {
namespace collectors {

struct CpuStats {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t guest_nice;

    uint64_t total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
    }

    uint64_t idle_total() const {
        return idle + iowait;
    }

    uint64_t non_idle() const {
        return user + nice + system + irq + softirq + steal + guest + guest_nice;
    }
};

class CpuCollector : public CollectorBase {
public:
    explicit CpuCollector(std::chrono::seconds interval);

protected:
    void collect() override;

private:
    std::vector<CpuStats> readCpuStats();
    double calculateCpuUsage(const CpuStats& prev, const CpuStats& curr);

    std::shared_ptr<metrics::GaugeMetric> cpu_usage_;
    std::vector<std::shared_ptr<metrics::GaugeMetric>> core_usage_;
    std::vector<CpuStats> prev_stats_;
};

}
}
