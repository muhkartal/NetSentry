# Troubleshooting Guide

This document provides solutions for common issues you might encounter when building or running NetSentry.

## Build Issues

### Template Implementation Errors

One of the most common build issues involves template implementation in separate .cpp files. In C++, template implementations generally need to be visible at the point of instantiation, which typically means they should be in header files.

#### Error: "No declaration matches..."

If you see errors like:

```
error: no declaration matches 'void netsentry::utils::Logger::log(netsentry::utils::LogLevel, const char*, T&&, Args&& ...)'
```

**Solution:** Move the template implementation to the header file:

1. In `logger.hpp`, add the implementation inline after the declaration:

```cpp
template<typename... Args>
inline void Logger::log(LogLevel level, const char* format, Args&&... args) {
    // Implementation here
}
```

2. Remove the implementation from `logger.cpp`

#### Error: "Binding reference of type... to 'const std::mutex' discards qualifiers"

This occurs when trying to lock a mutex in a const method:

```
error: binding reference of type 'std::lock_guard<std::mutex>::mutex_type&' to 'const std::mutex' discards qualifiers
```

**Solution:** Mark the mutex as `mutable` in the class definition:

```cpp
class Logger {
private:
    // Other members...
    mutable std::mutex mutex_;  // Add mutable keyword
};
```

### Complete Fix for Logger Implementation

Here's a complete fix for the logger implementation issues:

1. **Update logger.hpp**:

```cpp
// In the class definition:
mutable std::mutex mutex_;  // Add mutable keyword

// After the class definition:
// Add template implementations inline
template<typename... Args>
inline void Logger::log(LogLevel level, const char* format, Args&&... args) {
    if (level < current_level_) {
        return;
    }

    std::string message = formatString(format, std::forward<Args>(args)...);
    log(level, message.c_str());
}

// Add specialization for no arguments
template<>
inline void Logger::log<>(LogLevel level, const char* message) {
    // Implementation here
}

template<typename... Args>
inline std::string Logger::formatString(const char* format, Args&&... args) {
    // Implementation here
}
```

2. **Update logger.cpp**:

```cpp
// Remove template implementations, keep only non-template functions
```

## Runtime Issues

### Permission Issues with Packet Capture

If you encounter permission errors when trying to capture packets:

```
Failed to start packet capture on interface: eth0
```

**Solution:** Run NetSentry with elevated privileges:

```bash
sudo ./netsentry --interface eth0
```

### Database Connection Failed

If you see errors related to database initialization:

```
Failed to initialize database
```

**Solution:**

1. Check if the data directory exists and is writable
2. For SQLite, ensure you have permission to write to the database file
3. Configure a different database path in the config file

## Configuration Issues

### Unable to Start Web Server

If the web server fails to start:

```
Failed to start web server on port 9090
```

**Solution:**

1. Check if the port is already in use: `netstat -tuln | grep 9090`
2. Configure a different port in the config file
3. Stop any services using that port

## Platform-Specific Issues

### Linux

Most Linux distributions will work fine with the provided setup. Ensure you have libpcap installed:

```bash
# Debian/Ubuntu
sudo apt-get install -y libpcap-dev

# CentOS/RHEL
sudo yum install -y libpcap-devel
```

### macOS

On macOS, you can install dependencies with Homebrew:

```bash
brew install cmake boost libpcap
```

### Windows

Windows support is limited due to packet capture dependencies. Consider using:

1. Windows Subsystem for Linux (WSL2)
2. Docker Desktop for Windows
3. A custom build with WinPcap/Npcap instead of libpcap
