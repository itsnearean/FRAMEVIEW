#pragma once

#include <string_view>
#include <stdexcept>

namespace utils {

    void handle_error(const char* fmt, ...);
    [[noreturn]] void throw_runtime_error(const char* fmt, ...);
} // namespace utils 