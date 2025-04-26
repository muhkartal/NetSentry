#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

namespace netsentry {
namespace utils {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static void initialize(const std::string& filename, LogLevel level = LogLevel::INFO);
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    // Non-template function for simple case
    void log(LogLevel level, const char* message);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    const char* logLevelToString(LogLevel level) const;

    static Logger* instance_;
    static std::mutex instance_mutex_;

    std::string log_filename_;
    std::ofstream log_file_;
    LogLevel current_level_;
    mutable std::mutex mutex_;  // Must be mutable
};

// Macro definitions for easier logging
#define LOG_TRACE(message) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::TRACE, message)
#define LOG_DEBUG(message) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::DEBUG, message)
#define LOG_INFO(message) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::INFO, message)
#define LOG_WARNING(message) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::WARNING, message)
#define LOG_ERROR(message) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::ERROR, message)
#define LOG_CRITICAL(message) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::CRITICAL, message)

}
}
