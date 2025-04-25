#include "system_metrics.hpp"

namespace netsentry {
namespace metrics {

Metric::Metric(std::string name, MetricType type)
    : name_(std::move(name)), type_(type), last_updated_(std::chrono::system_clock::now()) {}

GaugeMetric::GaugeMetric(const std::string& name)
    : Metric(name, MetricType::GAUGE) {}

void GaugeMetric::update(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_value_ = value;
    last_updated_ = std::chrono::system_clock::now();
    historical_values_[last_updated_] = value;

    while (historical_values_.size() > 1000) {
        historical_values_.erase(historical_values_.begin());
    }
}

double GaugeMetric::getCurrentValue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_value_;
}

std::optional<double> GaugeMetric::getValueAt(const TimePoint& time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = historical_values_.lower_bound(time);
    if (it == historical_values_.end()) {
        if (historical_values_.empty()) {
            return std::nullopt;
        }
        return historical_values_.rbegin()->second;
    }

    return it->second;
}

CounterMetric::CounterMetric(const std::string& name)
    : Metric(name, MetricType::COUNTER) {}

void CounterMetric::update(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_value_ = value;
    last_updated_ = std::chrono::system_clock::now();
    historical_values_[last_updated_] = value;

    while (historical_values_.size() > 1000) {
        historical_values_.erase(historical_values_.begin());
    }
}

void CounterMetric::increment(double amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_value_ += amount;
    last_updated_ = std::chrono::system_clock::now();
    historical_values_[last_updated_] = current_value_;

    while (historical_values_.size() > 1000) {
        historical_values_.erase(historical_values_.begin());
    }
}

double CounterMetric::getCurrentValue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_value_;
}

std::optional<double> CounterMetric::getValueAt(const TimePoint& time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = historical_values_.lower_bound(time);
    if (it == historical_values_.end()) {
        if (historical_values_.empty()) {
            return std::nullopt;
        }
        return historical_values_.rbegin()->second;
    }

    return it->second;
}

}
}
