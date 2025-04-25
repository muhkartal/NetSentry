#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include "alert_rule.hpp"

namespace netsentry {
namespace alert {

using AlertCallback = std::function<void(const Alert&)>;

class AlertManager {
public:
    AlertManager();

    void createAlert(
        const std::string& name,
        std::unique_ptr<ConditionPolicy> condition,
        Severity severity = Severity::WARNING);

    void registerCallback(AlertCallback callback);

    void checkAlerts();

    std::vector<std::reference_wrapper<const Alert>> getAlerts() const;

private:
    std::vector<std::unique_ptr<Alert>> alerts_;
    std::vector<AlertCallback> callbacks_;
    CooldownPolicy cooldown_policy_;
    mutable std::mutex mutex_;
};

}
}
