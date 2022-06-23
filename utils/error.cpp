#include "error.h"
#include "logger.h"
#include <stdexcept>

namespace utils {

    void handle_error(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        utils::log_error(fmt, args);
        va_end(args);
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

// Variadic template for formatted errors

template<typename... Args>
void handle_error(std::string_view fmt, Args&&... args) {
    log_error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
[[noreturn]] void throw_runtime_error(std::string_view fmt, Args&&... args) {
    log_error(fmt, std::forward<Args>(args)...);
    throw std::runtime_error(utils::detail::format(fmt, std::forward<Args>(args)...));
}

} // namespace utils 