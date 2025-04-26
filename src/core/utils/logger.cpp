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

// Template implementations have been moved to the header file

}
}
