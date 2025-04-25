#include "alert_rule.hpp"

namespace netsentry {
namespace alert {

MetricThresholdCondition::MetricThresholdCondition(
    std::shared_ptr<metrics::Metric> metric,
    Comparator comparator,
    double threshold)
    : metric_(std::move(metric)), comparator_(comparator), threshold_(threshold) {}

bool MetricThresholdCondition::evaluate() const {
    if (!metric_) {
        return false;
    }

    double value = metric_->getCurrentValue();
    return compareValues(value, threshold_);
}

std::string MetricThresholdCondition::getDescription() const {
    if (!metric_) {
        return "Invalid metric";
    }

    std::string comparator_str;
    switch (comparator_) {
        case Comparator::GREATER_THAN:
            comparator_str = ">";
            break;
        case Comparator::LESS_THAN:
            comparator_str = "<";
            break;
        case Comparator::EQUAL_TO:
            comparator_str = "==";
            break;
        case Comparator::NOT_EQUAL_TO:
            comparator_str = "!=";
            break;
        case Comparator::GREATER_EQUAL:
            comparator_str = ">=";
            break;
        case Comparator::LESS_EQUAL:
            comparator_str = "<=";
            break;
    }

    return metric_->getName() + " " + comparator_str + " " + std::to_string(threshold_);
}

bool MetricThresholdCondition::compareValues(double value, double threshold) const {
    switch (comparator_) {
        case Comparator::GREATER_THAN:
            return value > threshold;
        case Comparator::LESS_THAN:
            return value < threshold;
        case Comparator::EQUAL_TO:
            return std::abs(value - threshold) < 1e-6;
        case Comparator::NOT_EQUAL_TO:
            return std::abs(value - threshold) >= 1e-6;
        case Comparator::GREATER_EQUAL:
            return value >= threshold;
        case Comparator::LESS_EQUAL:
            return value <= threshold;
        default:
            return false;
    }
}

CooldownPolicy::CooldownPolicy(std::chrono::seconds duration)
    : cooldown_duration_(duration) {}

bool CooldownPolicy::shouldSuppressAlert(const Alert& alert) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = last_fired_.find(alert.getName());
    if (it == last_fired_.end()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - it->second;

    return elapsed < cooldown_duration_;
}

void CooldownPolicy::recordAlertFired(const Alert& alert) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_fired_[alert.getName()] = std::chrono::steady_clock::now();
}

Alert::Alert(
    std::string name,
    std::unique_ptr<ConditionPolicy> condition,
    Severity severity)
    : name_(std::move(name)), condition_(std::move(condition)), severity_(severity) {}

std::string Alert::getMessage() const {
    std::string severity_str;
    switch (severity_) {
        case Severity::INFO:
            severity_str = "INFO";
            break;
        case Severity::WARNING:
            severity_str = "WARNING";
            break;
        case Severity::ERROR:
            severity_str = "ERROR";
            break;
        case Severity::CRITICAL:
            severity_str = "CRITICAL";
            break;
    }

    return "[" + severity_str + "] Alert: " + name_ + " - " + condition_->getDescription();
}

}
}
