#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include "../metrics/system_metrics.hpp"

namespace netsentry {
namespace collectors {

class CollectorBase {
public:
    explicit CollectorBase(std::chrono::milliseconds interval)
        : interval_(interval), running_(false) {}

    virtual ~CollectorBase() {
        stop();
    }

    CollectorBase(const CollectorBase&) = delete;
    CollectorBase& operator=(const CollectorBase&) = delete;

    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }

        running_ = true;
        worker_thread_ = std::thread(&CollectorBase::collectLoop, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                return;
            }

            running_ = false;
        }

        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    bool isRunning() const {
        return running_;
    }

    std::shared_ptr<metrics::Metric> getMetric(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::string> getMetricNames() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        names.reserve(metrics_.size());

        for (const auto& metric : metrics_) {
            names.push_back(metric.first);
        }

        return names;
    }

protected:
    virtual void collect() = 0;

    void registerMetric(std::shared_ptr<metrics::Metric> metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[metric->getName()] = std::move(metric);
    }

private:
    void collectLoop() {
        while (running_) {
            collect();

            auto sleepStart = std::chrono::steady_clock::now();
            while (running_ &&
                   (std::chrono::steady_clock::now() - sleepStart) < interval_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    std::chrono::milliseconds interval_;
    std::atomic<bool> running_;
    std::thread worker_thread_;

    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<metrics::Metric>> metrics_;
};

}
}
