#include "../src/handler/command_handler.hpp"
#include "../src/store/store.hpp"
#include <cassert>
#include <iostream>

void test_geoadd_returns_integer_one() {
    Store store;
    CommandHandler handler(store);

    std::string input = "*5\r\n$6\r\nGEOADD\r\n$6\r\nplaces\r\n$10\r\n11.5030378\r\n$9\r\n48."
                        "164271\r\n$6\r\nMunich\r\n";
    auto response = handler.process(input);

    assert(response == ":1\r\n");

    std::cout << "\u2713 Test passed: GEOADD responds with :1\r\n";
}

int main() {
    std::cout << "Running GEOADD command tests...\n\n";

    test_geoadd_returns_integer_one();

    std::cout << "\n\u2713 All tests passed!\n";
    return 0;
}
