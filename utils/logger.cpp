#include "logger.h"
#include <iostream>
#include <mutex>
#include <cstdio>
#include <ctime>

namespace utils {

namespace detail {
    inline std::mutex& log_mutex() {
        static std::mutex mtx;
        return mtx;
    }

    enum class log_level { info, warn, error, debug };

    constexpr const char* level_str(log_level level) {
        switch (level) {
            case log_level::info: return "INFO";
            case log_level::warn: return "WARN";
            case log_level::error: return "ERROR";
            case log_level::debug: return "DEBUG";
            default: return "LOG";
        }
    }

    constexpr const char* color_code(log_level level) {
        switch (level) {
            case log_level::info:  return "\033[32m"; // Green
            case log_level::warn:  return "\033[33m"; // Yellow
            case log_level::error: return "\033[31m"; // Red
            case log_level::debug: return "\033[34m"; // Blue
            default:               return "\033[0m";
        }
    }
    constexpr const char* reset_code() { return "\033[0m"; }

    inline std::string timestamp() {
        char buf[20];
        std::time_t t = std::time(nullptr);
        std::tm tm;
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return buf;
    }

    inline bool& debug_enabled_flag() {
        // default based on compile-time flag; can be overridden at runtime via set_debug_logging
#if defined(UTILS_DEBUG_LOGGING) && (UTILS_DEBUG_LOGGING==0)
        static bool enabled = false;
#else
        static bool enabled = true;
#endif
        return enabled;
    }

    void vlog(log_level level, const char* fmt, va_list args) {
        if (level == log_level::debug && !debug_enabled_flag()) return;
        std::lock_guard<std::mutex> lock(log_mutex());
        char msg[1024];
        std::vsnprintf(msg, sizeof(msg), fmt, args);
        std::cout << "[" << timestamp() << "] "
                  << color_code(level) << "[" << level_str(level) << "]" << reset_code()
                  << " " << msg << std::endl;
    }
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::vlog(detail::log_level::info, fmt, args);
    va_end(args);
}

void log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::vlog(detail::log_level::warn, fmt, args);
    va_end(args);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::vlog(detail::log_level::error, fmt, args);
    va_end(args);
}

void log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::vlog(detail::log_level::debug, fmt, args);
    va_end(args);
}

void set_debug_logging(bool enabled) {
    detail::debug_enabled_flag() = enabled;
}

bool is_debug_logging() {
    return detail::debug_enabled_flag();
}

} // namespace utils 

