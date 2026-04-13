#include "../src/handler/command_handler.hpp"
#include "../src/store/store.hpp"
#include <cassert>
#include <iostream>
#include <string>

static std::string make_geoadd_resp(std::string_view key,
                                    std::string_view lon,
                                    std::string_view lat,
                                    std::string_view member) {
    std::string input = "*5\r\n$6\r\nGEOADD\r\n";
    input += "$" + std::to_string(key.size()) + "\r\n" + std::string(key) + "\r\n";
    input += "$" + std::to_string(lon.size()) + "\r\n" + std::string(lon) + "\r\n";
    input += "$" + std::to_string(lat.size()) + "\r\n" + std::string(lat) + "\r\n";
    input += "$" + std::to_string(member.size()) + "\r\n" + std::string(member) + "\r\n";
    return input;
}

static bool is_resp_error(const std::string& resp) {
    return resp.starts_with("-") && resp.ends_with("\r\n");
}

static bool contains(std::string_view haystack, std::string_view needle) {
    return haystack.find(needle) != std::string_view::npos;
}

void test_geoadd_returns_integer_one() {
    Store store;
    CommandHandler handler(store);

    std::string input = "*5\r\n$6\r\nGEOADD\r\n$6\r\nplaces\r\n$10\r\n11.5030378\r\n$9\r\n48."
                        "164271\r\n$6\r\nMunich\r\n";
    auto response = handler.process(input);

    assert(response == ":1\r\n");

    std::cout << "\u2713 Test passed: GEOADD responds with :1\r\n";
}

void test_geoadd_invalid_longitude_returns_error() {
    Store store;
    CommandHandler handler(store);

    auto input = make_geoadd_resp("places", "181", "0.3", "test2");
    auto response = handler.process(input);

    assert(is_resp_error(response));
    assert(contains(response, "ERR"));
    assert(contains(response, "longitude"));

    std::cout << "\u2713 Test passed: GEOADD invalid longitude returns error\r\n";
}

void test_geoadd_invalid_latitude_returns_error() {
    Store store;
    CommandHandler handler(store);

    auto input = make_geoadd_resp("places", "10", "90", "test1");
    auto response = handler.process(input);

    assert(is_resp_error(response));
    assert(contains(response, "ERR"));
    assert(contains(response, "latitude"));

    std::cout << "\u2713 Test passed: GEOADD invalid latitude returns error\r\n";
}

void test_geoadd_both_invalid_returns_error_with_both_keywords() {
    Store store;
    CommandHandler handler(store);

    auto input = make_geoadd_resp("places", "200", "100", "test3");
    auto response = handler.process(input);

    assert(is_resp_error(response));
    assert(contains(response, "ERR"));
    assert(contains(response, "longitude"));
    assert(contains(response, "latitude"));

    std::cout << "\u2713 Test passed: GEOADD both invalid returns error with both keywords\r\n";
}

int main() {
    std::cout << "Running GEOADD command tests...\n\n";

    test_geoadd_returns_integer_one();
    test_geoadd_invalid_longitude_returns_error();
    test_geoadd_invalid_latitude_returns_error();
    test_geoadd_both_invalid_returns_error_with_both_keywords();

    std::cout << "\n\u2713 All tests passed!\n";
    return 0;
}
