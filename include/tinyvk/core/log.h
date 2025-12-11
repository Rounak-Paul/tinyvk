/**
 * @file log.h
 * @brief Logging utilities for TinyVK
 */

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <cstdarg>

namespace tvk {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

class Log {
public:
    static void SetLevel(LogLevel level) { s_Level = level; }
    static LogLevel GetLevel() { return s_Level; }

    template<typename... Args>
    static void Trace(const char* fmt, Args&&... args) {
        if (s_Level <= LogLevel::Trace)
            Print("[TRACE]", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Debug(const char* fmt, Args&&... args) {
        if (s_Level <= LogLevel::Debug)
            Print("[DEBUG]", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Info(const char* fmt, Args&&... args) {
        if (s_Level <= LogLevel::Info)
            Print("[INFO]", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(const char* fmt, Args&&... args) {
        if (s_Level <= LogLevel::Warn)
            Print("[WARN]", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(const char* fmt, Args&&... args) {
        if (s_Level <= LogLevel::Error)
            Print("[ERROR]", fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Fatal(const char* fmt, Args&&... args) {
        if (s_Level <= LogLevel::Fatal)
            Print("[FATAL]", fmt, std::forward<Args>(args)...);
    }

private:
    static inline LogLevel s_Level = LogLevel::Trace;

    template<typename... Args>
    static void Print(const char* prefix, const char* fmt, Args&&... args) {
        std::ostringstream oss;
        oss << prefix << " ";
        FormatTo(oss, fmt, std::forward<Args>(args)...);
        std::cout << oss.str() << std::endl;
    }

    static void FormatTo(std::ostringstream& oss, const char* fmt) {
        oss << fmt;
    }

    template<typename T, typename... Args>
    static void FormatTo(std::ostringstream& oss, const char* fmt, T&& value, Args&&... args) {
        while (*fmt) {
            if (*fmt == '{' && *(fmt + 1) == '}') {
                oss << std::forward<T>(value);
                FormatTo(oss, fmt + 2, std::forward<Args>(args)...);
                return;
            }
            oss << *fmt++;
        }
    }
};

// Convenience macros
#define TVK_LOG_TRACE(...) ::tvk::Log::Trace(__VA_ARGS__)
#define TVK_LOG_DEBUG(...) ::tvk::Log::Debug(__VA_ARGS__)
#define TVK_LOG_INFO(...)  ::tvk::Log::Info(__VA_ARGS__)
#define TVK_LOG_WARN(...)  ::tvk::Log::Warn(__VA_ARGS__)
#define TVK_LOG_ERROR(...) ::tvk::Log::Error(__VA_ARGS__)
#define TVK_LOG_FATAL(...) ::tvk::Log::Fatal(__VA_ARGS__)

#ifdef TVK_DEBUG_BUILD
    #define TVK_ASSERT(condition, ...) \
        do { \
            if (!(condition)) { \
                TVK_LOG_FATAL("Assertion failed: {}", #condition); \
                TVK_LOG_FATAL(__VA_ARGS__); \
                std::abort(); \
            } \
        } while (false)
#else
    #define TVK_ASSERT(condition, ...) ((void)0)
#endif

} // namespace tvk
