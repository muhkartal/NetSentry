#pragma once

#include <chrono>
#include <memory>
#include "collector_base.hpp"

namespace netsentry {
namespace collectors {

struct MemoryStats {
    uint64_t total;
    uint64_t available;
    uint64_t used;
    uint64_t free;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swap_total;
    uint64_t swap_used;
    uint64_t swap_free;
};

class MemoryCollector : public CollectorBase {
public:
    explicit MemoryCollector(std::chrono::seconds interval);

protected:
    void collect() override;

private:
    MemoryStats readMemoryStats();

    std::shared_ptr<metrics::GaugeMetric> memory_total_;
    std::shared_ptr<metrics::GaugeMetric> memory_used_;
    std::shared_ptr<metrics::GaugeMetric> memory_free_;
    std::shared_ptr<metrics::GaugeMetric> memory_usage_percent_;
    std::shared_ptr<metrics::GaugeMetric> swap_total_;
    std::shared_ptr<metrics::GaugeMetric> swap_used_;
    std::shared_ptr<metrics::GaugeMetric> swap_usage_percent_;
};

}
}
