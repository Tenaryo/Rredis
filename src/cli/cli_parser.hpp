#pragma once

#include "server/server_config.hpp"
#include <cstring>
#include <stdexcept>

inline int parse_port(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            return std::stoi(argv[i + 1]);
        }
    }
    return 6379;
}

inline std::optional<ReplicaOfConfig> parse_replicaof(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--replicaof") == 0 && i + 1 < argc) {
            std::string val = argv[i + 1];
            auto space = val.find(' ');
            if (space != std::string::npos) {
                return ReplicaOfConfig{val.substr(0, space), std::stoi(val.substr(space + 1))};
            }
        }
    }
    return std::nullopt;
}
