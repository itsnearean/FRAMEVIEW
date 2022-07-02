#pragma once

#include <cstdarg>

namespace utils {

// printf-style logging: log_info("value: %d, %s", 42, "foo");
void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_error(const char* fmt, ...);
void log_debug(const char* fmt, ...);

// control debug logging verbosity
void set_debug_logging(bool enabled);
bool is_debug_logging();

} // namespace utils 