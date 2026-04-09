#include "../src/handler/command_handler.hpp"
#include "../src/store/store.hpp"
#include <cassert>
#include <iostream>

void test_wait_zero_replicas_no_connected() {
    Store store;
    CommandHandler handler(store);

    auto result = handler.process("*3\r\n$4\r\nWAIT\r\n$1\r\n0\r\n$5\r\n60000\r\n");

    assert(result == ":0\r\n");

    std::cout << "✓ Test 1 passed: WAIT 0 with no replicas returns 0\n";
}

void test_wait_missing_arguments() {
    Store store;
    CommandHandler handler(store);

    auto result = handler.process("*1\r\n$4\r\nWAIT\r\n");
    assert(result.find("ERR wrong number of arguments") != std::string::npos);

    result = handler.process("*2\r\n$4\r\nWAIT\r\n$1\r\n0\r\n");
    assert(result.find("ERR wrong number of arguments") != std::string::npos);

    std::cout << "✓ Test 2 passed: WAIT with missing arguments returns error\n";
}

int main() {
    std::cout << "Running WAIT command tests...\n\n";

    test_wait_zero_replicas_no_connected();
    test_wait_missing_arguments();

    std::cout << "\n✓ All tests passed!\n";
    return 0;
}
