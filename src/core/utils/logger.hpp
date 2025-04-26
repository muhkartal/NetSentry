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

    template<typename... Args>
    void log(LogLevel level, const char* format, Args&&... args);

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename... Args>
    std::string formatString(const char* format, Args&&... args);

    const char* logLevelToString(LogLevel level) const;

    static Logger* instance_;
    static std::mutex instance_mutex_;

    std::string log_filename_;
    std::ofstream log_file_;
    LogLevel current_level_;
    mutable std::mutex mutex_; // Fix: Made mutex mutable to use in const methods
};

#define LOG_TRACE(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::CRITICAL, __VA_ARGS__)

}
}
