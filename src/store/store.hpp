#pragma once

#include <optional>
#include <string>
#include <unordered_map>

class Store {
    std::unordered_map<std::string, std::string> data_;
  public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool exists(const std::string& key) const;
    bool del(const std::string& key);
};
