#pragma once

#include <cstdarg>

namespace utils {

// printf-style logging: log_info("value: %d, %s", 42, "foo");
void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_error(const char* fmt, ...);
void log_debug(const char* fmt, ...);

} // namespace utils 