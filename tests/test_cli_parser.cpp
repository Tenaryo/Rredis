#include "../src/cli/cli_parser.hpp"
#include <cassert>
#include <iostream>

void test_parse_port_custom() {
    char arg0[] = "redis";
    char arg1[] = "--port";
    char arg2[] = "6380";
    char* argv[] = {arg0, arg1, arg2};
    int argc = 3;

    int port = parse_port(argc, argv);

    assert(port == 6380);

    std::cout << "Test 1 passed: parse_port returns 6380 for --port 6380\n";
}

void test_parse_port_default() {
    char arg0[] = "redis";
    char* argv[] = {arg0};
    int argc = 1;

    int port = parse_port(argc, argv);

    assert(port == 6379);

    std::cout << "Test 2 passed: parse_port returns 6379 when no --port provided\n";
}

int main() {
    std::cout << "Running CLI parser tests...\n\n";

    test_parse_port_custom();
    test_parse_port_default();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
