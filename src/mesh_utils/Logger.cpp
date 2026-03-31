#include "mesh_core/Logger.hpp"
#include <Arduino.h>
#include <cstdio>

namespace mesh {

bool Logger::initialized_ = false;

void Logger::init(unsigned long baudRate) {
    if (!initialized_) {
        Serial.begin(baudRate);
        while (!Serial) { delay(10); }
        initialized_ = true;
    }
}

void Logger::info(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log("INFO", tag, fmt, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log("WARN", tag, fmt, args);
    va_end(args);
}

void Logger::error(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log("ERROR", tag, fmt, args);
    va_end(args);
}

void Logger::log(const char* level, const char* tag, const char* fmt, va_list args) {
    if (!initialized_) init();
    char buf[256];
    int offset = snprintf(buf, sizeof(buf), "[%s] [%s] ", level, tag);
    if (offset > 0 && offset < (int)sizeof(buf)) {
        vsnprintf(buf + offset, sizeof(buf) - offset, fmt, args);
    }
    Serial.println(buf);
}

} // namespace mesh
