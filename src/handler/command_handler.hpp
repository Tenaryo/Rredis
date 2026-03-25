#pragma once

#include <string>
#include <string_view>

class Store;

class CommandHandler {
    Store& store_;
  public:
    explicit CommandHandler(Store& store);

    std::string process(std::string_view input);
  private:
    static std::string handle_ping();
    static std::string handle_echo(std::string_view args);
    std::string handle_set(const std::string& key, const std::string& value);
    std::string handle_get(const std::string& key);
};
