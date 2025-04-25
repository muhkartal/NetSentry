#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <mutex>

namespace netsentry {
namespace metrics {

enum class MetricType {
    GAUGE,
    COUNTER,
    HISTOGRAM
};

class Metric {
public:
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

    Metric(std::string name, MetricType type);
    virtual ~Metric() = default;

    const std::string& getName() const { return name_; }
    MetricType getType() const { return type_; }

    virtual void update(double value) = 0;
    virtual double getCurrentValue() const = 0;
    virtual std::optional<double> getValueAt(const TimePoint& time) const = 0;

protected:
    std::string name_;
    MetricType type_;
    TimePoint last_updated_{};
    mutable std::mutex mutex_;
};

class GaugeMetric : public Metric {
public:
    GaugeMetric(const std::string& name);

    void update(double value) override;
    double getCurrentValue() const override;
    std::optional<double> getValueAt(const TimePoint& time) const override;

private:
    double current_value_{0.0};
    std::map<TimePoint, double> historical_values_;
};

class CounterMetric : public Metric {
public:
    CounterMetric(const std::string& name);

    void update(double value) override;
    void increment(double amount = 1.0);
    double getCurrentValue() const override;
    std::optional<double> getValueAt(const TimePoint& time) const override;

private:
    double current_value_{0.0};
    std::map<TimePoint, double> historical_values_;
};

}
}
