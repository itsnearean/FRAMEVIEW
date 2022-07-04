#include "error.h"
#include "logger.h"
#include <stdexcept>
#include <cstdarg>
#include <cstdio>
#include <string_view>

namespace utils {

void handle_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char msg[1024];
    std::vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    utils::log_error("%s", msg);
}

[[noreturn]] void throw_runtime_error(const char* fmt, ...) {
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    utils::log_error("%s", msg);
    throw std::runtime_error(msg);
}

// templates
template<typename... Args>
void handle_error(std::string_view fmt, Args&&... args) {
    utils::log_error(fmt.data(), std::forward<Args>(args)...);
}

template<typename... Args>
[[noreturn]] void throw_runtime_error(std::string_view fmt, Args&&... args) {
    char msg[1024];
    std::snprintf(msg, sizeof(msg), fmt.data(), std::forward<Args>(args)...);
    utils::log_error("%s", msg);
    throw std::runtime_error(msg);
}

} // namespace utils