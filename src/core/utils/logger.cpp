#include "logger.hpp"
#include <cstdio>
#include <ctime>
#include <cstring>

namespace netsentry {
namespace utils {

Logger* Logger::instance_ = nullptr;
std::mutex Logger::instance_mutex_;

void Logger::initialize(const std::string& filename, LogLevel level) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        instance_ = new Logger();
        instance_->log_filename_ = filename;
        instance_->log_file_.open(filename, std::ios::out | std::ios::app);
        instance_->current_level_ = level;
    }
}

Logger& Logger::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        instance_ = new Logger();
        instance_->log_filename_ = "netsentry.log";
        instance_->log_file_.open(instance_->log_filename_, std::ios::out | std::ios::app);
        instance_->current_level_ = LogLevel::INFO;
    }
    return *instance_;
}

Logger::Logger() {}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_level_ = level;
}

LogLevel Logger::getLogLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_level_;
}

const char* Logger::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE:    return "TRACE";
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

template<>
void Logger::log<>(LogLevel level, const char* message) {
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
void Logger::log(LogLevel level, const char* format, T&& arg, Args&&... args) {
    if (level < current_level_) {
        return;
    }

    std::string message = formatString(format, std::forward<T>(arg), std::forward<Args>(args)...);
    log(level, message.c_str());
}

template<typename T>
std::string Logger::formatString(const char* format, T&& arg) {
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
std::string Logger::formatString(const char* format, T&& arg, Args&&... args) {
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

}
}
