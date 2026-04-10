#include "mesh_core/Logger.hpp"
#include <Arduino.h>
#include <cstdio>

namespace mesh {

bool Logger::initialized_ = false;
LogLevel Logger::minLevel_ = LogLevel::INFO;

void Logger::init(unsigned long baudRate) {
    if (!initialized_) {
        // Only call begin if not already initialized by user
        if (!Serial) {
            Serial.begin(baudRate);
            delay(10);
        }
        initialized_ = true;
    }
}

void Logger::setLogLevel(LogLevel level) {
    minLevel_ = level;
}

void Logger::debug(const char* tag, const char* fmt, ...) {
    if (minLevel_ > LogLevel::DEBUG) return;
    va_list args;
    va_start(args, fmt);
    log(LogLevel::DEBUG, tag, fmt, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* fmt, ...) {
    if (minLevel_ > LogLevel::INFO) return;
    va_list args;
    va_start(args, fmt);
    log(LogLevel::INFO, tag, fmt, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* fmt, ...) {
    if (minLevel_ > LogLevel::WARN) return;
    va_list args;
    va_start(args, fmt);
    log(LogLevel::WARN, tag, fmt, args);
    va_end(args);
}

void Logger::error(const char* tag, const char* fmt, ...) {
    if (minLevel_ > LogLevel::ERROR) return;
    va_list args;
    va_start(args, fmt);
    log(LogLevel::ERROR, tag, fmt, args);
    va_end(args);
}

void Logger::log(LogLevel level, const char* tag, const char* fmt, va_list args) {
    if (!initialized_) init();
    
    const char* lvlStr = "INFO";
    switch (level) {
        case LogLevel::DEBUG: lvlStr = "DEBUG"; break;
        case LogLevel::INFO:  lvlStr = "INFO";  break;
        case LogLevel::WARN:  lvlStr = "WARN";  break;
        case LogLevel::ERROR: lvlStr = "ERROR"; break;
    }

    char buf[256];
    int offset = snprintf(buf, sizeof(buf), "[%s] [%s] ", lvlStr, tag);
    if (offset > 0 && offset < (int)sizeof(buf)) {
        vsnprintf(buf + offset, sizeof(buf) - offset, fmt, args);
    }
    Serial.println(buf);
}

} // namespace mesh
