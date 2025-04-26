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
    mutable std::mutex mutex_;  // Changed to mutable to fix const method issue
};

// Template implementations need to be in the header file
template<typename... Args>
inline void Logger::log(LogLevel level, const char* format, Args&&... args) {
    if (level < current_level_) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now{};

#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif

    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_now);

    // Format the message
    char message_buffer[1024];
    snprintf(message_buffer, sizeof(message_buffer), format, std::forward<Args>(args)...);

    std::lock_guard<std::mutex> lock(mutex_);
    log_file_ << "[" << timestamp << "] [" << logLevelToString(level) << "] " << message_buffer << std::endl;
}

template<typename... Args>
inline std::string Logger::formatString(const char* format, Args&&... args) {
    char buffer[1024];
    int result = snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);

    if (result < 0) {
        return "Error formatting string";
    }

    return std::string(buffer, result);
}

#define LOG_TRACE(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::CRITICAL, __VA_ARGS__)

}
}
