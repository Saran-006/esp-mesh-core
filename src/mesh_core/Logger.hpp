#pragma once

#include <cstdarg>
#include <cstdint>

namespace mesh {

enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3
};

class Logger {
public:
    static void init(unsigned long baudRate = 115200);
    static void setLogLevel(LogLevel level);
    static void debug(const char* tag, const char* fmt, ...);
    static void info(const char* tag, const char* fmt, ...);
    static void warn(const char* tag, const char* fmt, ...);
    static void error(const char* tag, const char* fmt, ...);
private:
    static void log(LogLevel level, const char* tag, const char* fmt, va_list args);
    static bool initialized_;
    static LogLevel minLevel_;
};

} // namespace mesh

#define LOG_DEBUG(tag, fmt, ...) mesh::Logger::debug(tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...)  mesh::Logger::info(tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)  mesh::Logger::warn(tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) mesh::Logger::error(tag, fmt, ##__VA_ARGS__)
