#include "command_handler.hpp"
#include "protocol/resp_parser.hpp"
#include "store/store.hpp"
#include <algorithm>
#include <cctype>

CommandHandler::CommandHandler(Store& store) : store_(store) {}

std::string CommandHandler::process(std::string_view input) {
    auto parsed = RespParser::parse(input);
    if (!parsed) {
        return RespParser::encode_error("ERR " + parsed.error());
    }

    const auto& args = *parsed;
    if (args.empty()) {
        return RespParser::encode_error("ERR empty command");
    }

    std::string cmd = args[0];
    std::transform(
        cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return std::toupper(c); });

    if (cmd == "PING") {
        return handle_ping();
    } else if (cmd == "ECHO") {
        if (args.size() < 2) {
            return RespParser::encode_error("ERR wrong number of arguments for 'echo' command");
        }
        return handle_echo(args[1]);
    } else if (cmd == "SET") {
        if (args.size() < 3) {
            return RespParser::encode_error("ERR wrong number of arguments for 'set' command");
        }
        return handle_set(args[1], args[2]);
    } else if (cmd == "GET") {
        if (args.size() < 2) {
            return RespParser::encode_error("ERR wrong number of arguments for 'get' command");
        }
        return handle_get(args[1]);
    }

    return RespParser::encode_error("ERR unknown command '" + cmd + "'");
}

std::string CommandHandler::handle_ping() { return RespParser::encode_simple_string("PONG"); }

std::string CommandHandler::handle_echo(std::string_view args) {
    return RespParser::encode_bulk_string(args);
}

std::string CommandHandler::handle_set(const std::string& key, const std::string& value) {
    store_.set(key, value);
    return RespParser::encode_simple_string("OK");
}

std::string CommandHandler::handle_get(const std::string& key) {
    auto value = store_.get(key);
    if (value) {
        return RespParser::encode_bulk_string(*value);
    }
    return RespParser::encode_null_bulk_string();
}
