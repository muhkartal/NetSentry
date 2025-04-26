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
    mutable std::mutex mutex_; // Added 'mutable' here
};

// Template implementations - move from .cpp to .hpp
template<>
inline void Logger::log<>(LogLevel level, const char* message) {
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

    std::lock_guard<std::mutex> lock(mutex_);
    log_file_ << "[" << timestamp << "] [" << logLevelToString(level) << "] " << message << std::endl;
}

template<typename T, typename... Args>
inline void Logger::log(LogLevel level, const char* format, T&& arg, Args&&... args) {
    if (level < current_level_) {
        return;
    }

    std::string message = formatString(format, std::forward<T>(arg), std::forward<Args>(args)...);
    log(level, message.c_str());
}

template<typename T>
inline std::string Logger::formatString(const char* format, T&& arg) {
    size_t buf_size = 1024;
    std::string result;
    result.resize(buf_size);

    int retval = snprintf(&result[0], buf_size, format, std::forward<T>(arg));
    if (retval < 0) {
        return "Error formatting string";
    }

    result.resize(retval);
    return result;
}

template<typename T, typename... Args>
inline std::string Logger::formatString(const char* format, T&& arg, Args&&... args) {
    size_t buf_size = 1024;
    std::string result;
    result.resize(buf_size);

    int retval = snprintf(&result[0], buf_size, format, std::forward<T>(arg), std::forward<Args>(args)...);
    if (retval < 0) {
        return "Error formatting string";
    }

    result.resize(retval);
    return result;
}

#define LOG_TRACE(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::CRITICAL, __VA_ARGS__)

}
}
