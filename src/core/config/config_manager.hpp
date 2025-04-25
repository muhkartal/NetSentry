#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <any>
#include <typeindex>
#include <mutex>
#include <memory>
#include <fstream>
#include <stdexcept>

namespace netsentry {
namespace config {

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename);

    void setDefaultConfig();

    template<typename T>
    std::optional<T> get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = values_.find(key);
        if (it == values_.end()) {
            return std::nullopt;
        }

        if (std::type_index(typeid(T)) != it->second.first) {
            return std::nullopt;
        }

        return std::any_cast<T>(it->second.second);
    }

    template<typename T>
    T getOrDefault(const std::string& key, const T& default_value) const {
        auto value = get<T>(key);
        return value.has_value() ? *value : default_value;
    }

    template<typename T>
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[key] = std::make_pair(std::type_index(typeid(T)), std::any(value));
    }

    bool exists(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.find(key) != values_.end();
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.erase(key);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.clear();
    }

    std::vector<std::string> getAllKeys() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> keys;
        keys.reserve(values_.size());

        for (const auto& pair : values_) {
            keys.push_back(pair.first);
        }

        return keys;
    }

private:
    ConfigManager();

    std::unordered_map<std::string, std::pair<std::type_index, std::any>> values_;
    mutable std::mutex mutex_;

    bool parseYaml(const std::string& content);
    std::string generateYaml() const;

    template<typename T>
    void parseYamlValue(const std::string& key, const std::string& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            // Remove quotes if present
            std::string unquoted = value;
            if (unquoted.size() >= 2 &&
                ((unquoted.front() == '"' && unquoted.back() == '"') ||
                 (unquoted.front() == '\'' && unquoted.back() == '\''))) {
                unquoted = unquoted.substr(1, unquoted.size() - 2);
            }
            set<std::string>(key, unquoted);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            if (value == "true" || value == "yes" || value == "on" || value == "1") {
                set<bool>(key, true);
            } else if (value == "false" || value == "no" || value == "off" || value == "0") {
                set<bool>(key, false);
            }
        }
        else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            try {
                set<T>(key, static_cast<T>(std::stoll(value)));
            } catch (...) {}
        }
        else if constexpr (std::is_floating_point_v<T>) {
            try {
                set<T>(key, static_cast<T>(std::stod(value)));
            } catch (...) {}
        }
    }
};

}
}
