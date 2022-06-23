#include "../utils/error.h"
#include <cassert>
#include <iostream>

void test_error() {
    // This should log an error but not throw
    utils::handle_error("Test handle_error message");
    utils::handle_error("Formatted error: %d, %s", 123, "abc");

    // This should throw
    bool threw = false;
    try {
        utils::throw_runtime_error("Test throw_runtime_error");
    } catch (const std::runtime_error& e) {
        threw = true;
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    assert(threw);

    threw = false;
    try {
        utils::throw_runtime_error("Formatted throw: %d, %s", 456, "def");
    } catch (const std::runtime_error& e) {
        threw = true;
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
    assert(threw);
}

int main() {
    test_error();
    std::cout << "Error tests completed. (Check output above)" << std::endl;
    return 0;
} 