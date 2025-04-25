#include "alert_manager.hpp"

namespace netsentry {
namespace alert {

AlertManager::AlertManager()
    : cooldown_policy_(std::chrono::seconds(60)) {}

void AlertManager::createAlert(
    const std::string& name,
    std::unique_ptr<ConditionPolicy> condition,
    Severity severity) {

    std::lock_guard<std::mutex> lock(mutex_);
    auto alert = std::make_unique<Alert>(name, std::move(condition), severity);
    alerts_.push_back(std::move(alert));
}

void AlertManager::registerCallback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(callback));
}

void AlertManager::checkAlerts() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& alert : alerts_) {
        if (alert->check() && !cooldown_policy_.shouldSuppressAlert(*alert)) {
            cooldown_policy_.recordAlertFired(*alert);

            for (const auto& callback : callbacks_) {
                callback(*alert);
            }
        }
    }
}

std::vector<std::reference_wrapper<const Alert>> AlertManager::getAlerts() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::reference_wrapper<const Alert>> result;
    result.reserve(alerts_.size());

    for (const auto& alert : alerts_) {
        result.emplace_back(*alert);
    }

    return result;
}

}
}#include "alert_manager.hpp"

namespace netsentry {
namespace alert {

AlertManager::AlertManager()
    : cooldown_policy_(std::chrono::seconds(60)) {}

void AlertManager::createAlert(
    const std::string& name,
    std::unique_ptr<ConditionPolicy> condition,
    Severity severity) {

    std::lock_guard<std::mutex> lock(mutex_);
    auto alert = std::make_unique<Alert>(name, std::move(condition), severity);
    alerts_.push_back(std::move(alert));
}

void AlertManager::registerCallback(AlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(callback));
}

void AlertManager::checkAlerts() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& alert : alerts_) {
        if (alert->check() && !cooldown_policy_.shouldSuppressAlert(*alert)) {
            cooldown_policy_.recordAlertFired(*alert);

            for (const auto& callback : callbacks_) {
                callback(*alert);
            }
        }
    }
}

std::vector<std::reference_wrapper<const Alert>> AlertManager::getAlerts() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::reference_wrapper<const Alert>> result;
    result.reserve(alerts_.size());

    for (const auto& alert : alerts_) {
        result.emplace_back(*alert);
    }

    return result;
}

}
}
