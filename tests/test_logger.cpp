#include "../utils/logger.h"
#include <cassert>
#include <iostream>

void test_logger() {
    // These should print to stdout; visually inspect output
    utils::log_info("Test info message");
    utils::log_warn("Test warn message");
    utils::log_error("Test error message");
    utils::log_debug("Test debug message");
    utils::log_info("Formatted: %d, %s", 42, "hello");
    utils::log_warn("Warn: %f", 3.14);
    utils::log_error("Error: %x", 255);
    utils::log_debug("Debug: %c", 'A');
}

int main() {
    test_logger();
    std::cout << "Logger tests completed. (Check output above)" << std::endl;
    return 0;
} 