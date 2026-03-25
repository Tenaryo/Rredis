#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

class Store {
    struct Entry {
        std::string value;
        std::optional<std::chrono::steady_clock::time_point> expiry;
    };

    std::unordered_map<std::string, Entry> data_;

    static std::chrono::steady_clock::time_point get_current_time();
  public:
    void set(const std::string& key,
             const std::string& value,
             std::optional<uint64_t> ttl_ms = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    bool exists(const std::string& key);
    bool del(const std::string& key);
};
