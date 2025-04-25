#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <memory>
#include <variant>
#include <optional>
#include <map>
#include <mutex>

#include "../core/metrics/system_metrics.hpp"

namespace netsentry {
namespace alert {

enum class Severity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class Comparator {
    GREATER_THAN,
    LESS_THAN,
    EQUAL_TO,
    NOT_EQUAL_TO,
    GREATER_EQUAL,
    LESS_EQUAL
};

class Alert;

class ConditionPolicy {
public:
    virtual ~ConditionPolicy() = default;
    virtual bool evaluate() const = 0;
    virtual std::string getDescription() const = 0;
};

class MetricThresholdCondition : public ConditionPolicy {
public:
    MetricThresholdCondition(
        std::shared_ptr<metrics::Metric> metric,
        Comparator comparator,
        double threshold);

    bool evaluate() const override;
    std::string getDescription() const override;

private:
    std::shared_ptr<metrics::Metric> metric_;
    Comparator comparator_;
    double threshold_;

    bool compareValues(double value, double threshold) const;
};

class CooldownPolicy {
public:
    explicit CooldownPolicy(std::chrono::seconds duration);

    bool shouldSuppressAlert(const Alert& alert) const;
    void recordAlertFired(const Alert& alert);

private:
    std::chrono::seconds cooldown_duration_;
    mutable std::map<std::string, std::chrono::steady_clock::time_point> last_fired_;
    mutable std::mutex mutex_;
};

class Alert {
public:
    Alert(
        std::string name,
        std::unique_ptr<ConditionPolicy> condition,
        Severity severity = Severity::WARNING);

    const std::string& getName() const { return name_; }
    Severity getSeverity() const { return severity_; }
    const ConditionPolicy& getCondition() const { return *condition_; }

    bool check() const { return condition_->evaluate(); }

    std::string getMessage() const;

private:
    std::string name_;
    std::unique_ptr<ConditionPolicy> condition_;
    Severity severity_;
};

}
}
