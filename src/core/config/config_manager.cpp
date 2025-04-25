#include "config_manager.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

namespace netsentry {
namespace config {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    setDefaultConfig();
}

void ConfigManager::setDefaultConfig() {
    set<bool>("enable_api", false);
    set<uint16_t>("api_port", 8080);

    set<bool>("enable_web", false);
    set<uint16_t>("web_port", 9090);

    set<bool>("enable_packet_capture", false);
    set<std::string>("capture_interface", "eth0");

    set<std::string>("log_level", "info");
    set<std::string>("log_file", "netsentry.log");

    set<uint32_t>("metric_retention_seconds", 3600);
    set<uint32_t>("alert_cooldown_seconds", 60);

    set<uint32_t>("cpu_threshold_warning", 80);
    set<uint32_t>("cpu_threshold_critical", 90);

    set<uint32_t>("memory_threshold_warning", 75);
    set<uint32_t>("memory_threshold_critical", 85);
}

bool ConfigManager::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return parseYaml(buffer.str());
}

bool ConfigManager::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << generateYaml();
    return true;
}

bool ConfigManager::parseYaml(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    std::regex key_value_regex(R"(^\s*([^:]+)\s*:\s*(.*)$)");
    std::smatch matches;

    while (std::getline(stream, line)) {
        // Skip comments and empty lines
        if (line.empty() || line.front() == '#') {
            continue;
        }

        if (std::regex_match(line, matches, key_value_regex)) {
            if (matches.size() == 3) {
                std::string key = matches[1].str();
                std::string value = matches[2].str();

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Try to parse as different types
                if (value == "true" || value == "false" ||
                    value == "yes" || value == "no" ||
                    value == "on" || value == "off") {
                    parseYamlValue<bool>(key, value);
                } else if (std::regex_match(value, std::regex(R"(^-?\d+$)"))) {
                    parseYamlValue<int64_t>(key, value);
                } else if (std::regex_match(value, std::regex(R"(^-?\d+\.\d+$)"))) {
                    parseYamlValue<double>(key, value);
                } else {
                    parseYamlValue<std::string>(key, value);
                }
            }
        }
    }

    return true;
}

std::string ConfigManager::generateYaml() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream yaml;
    yaml << "# NetSentry Configuration\n\n";

    // Group settings by category
    std::unordered_map<std::string, std::vector<std::string>> categories;

    for (const auto& key : getAllKeys()) {
        std::string category = "general";
        size_t pos = key.find('_');
        if (pos != std::string::npos) {
            category = key.substr(0, pos);
        }

        categories[category].push_back(key);
    }

    for (const auto& [category, keys] : categories) {
        yaml << "# " << category << " settings\n";

        for (const auto& key : keys) {
            auto it = values_.find(key);
            if (it != values_.end()) {
                yaml << key << ": ";

                const std::type_index& type = it->second.first;
                const std::any& value = it->second.second;

                if (type == std::type_index(typeid(std::string))) {
                    yaml << "\"" << std::any_cast<std::string>(value) << "\"";
                } else if (type == std::type_index(typeid(bool))) {
                    yaml << (std::any_cast<bool>(value) ? "true" : "false");
                } else if (type == std::type_index(typeid(int64_t))) {
                    yaml << std::any_cast<int64_t>(value);
                } else if (type == std::type_index(typeid(int32_t))) {
                    yaml << std::any_cast<int32_t>(value);
                } else if (type == std::type_index(typeid(uint64_t))) {
                    yaml << std::any_cast<uint64_t>(value);
                } else if (type == std::type_index(typeid(uint32_t))) {
                    yaml << std::any_cast<uint32_t>(value);
                } else if (type == std::type_index(typeid(uint16_t))) {
                    yaml << std::any_cast<uint16_t>(value);
                } else if (type == std::type_index(typeid(double))) {
                    yaml << std::any_cast<double>(value);
                } else if (type == std::type_index(typeid(float))) {
                    yaml << std::any_cast<float>(value);
                }

                yaml << "\n";
            }
        }

        yaml << "\n";
    }

    return yaml.str();
}

}
}
