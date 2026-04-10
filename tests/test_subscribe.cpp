#include "../src/handler/command_handler.hpp"
#include "../src/protocol/resp_parser.hpp"
#include "../src/store/store.hpp"
#include <cassert>
#include <iostream>

void test_subscribe_single_channel() {
    Store store;
    CommandHandler handler(store);

    auto response = handler.process("*2\r\n$9\r\nsubscribe\r\n$3\r\nfoo\r\n");

    auto expected = RespParser::encode_array({"subscribe", "foo", "1"});
    assert(response == expected);

    std::cout << "Test 1 passed: SUBSCRIBE foo returns [\"subscribe\", \"foo\", 1]\n";
}

int main() {
    std::cout << "Running SUBSCRIBE tests...\n\n";

    test_subscribe_single_channel();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
