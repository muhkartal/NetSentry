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

    template<typename T, typename... Args>
    void log(LogLevel level, const char* format, T&& arg, Args&&... args) {
        if (level < current_level_) {
            return;
        }

        std::string message = formatString(format, std::forward<T>(arg), std::forward<Args>(args)...);
        log(level, message.c_str());
    }

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel();

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename T>
    std::string formatString(const char* format, T&& arg) {
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
    std::string formatString(const char* format, T&& arg, Args&&... args) {
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

    const char* logLevelToString(LogLevel level) const;

    static Logger* instance_;
    static std::mutex instance_mutex_;

    std::string log_filename_;
    std::ofstream log_file_;
    LogLevel current_level_;
    std::mutex mutex_;
};

#define LOG_TRACE(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARNING(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::WARNING, __VA_ARGS__)
#define LOG_ERROR(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) ::netsentry::utils::Logger::getInstance().log(::netsentry::utils::LogLevel::CRITICAL, __VA_ARGS__)

}
}
