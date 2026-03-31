#pragma once

#include <optional>
#include <string>

struct ReplicaOfConfig {
    std::string host;
    int port;
};

struct ServerConfig {
    std::optional<ReplicaOfConfig> replicaof;
    bool is_replica() const { return replicaof.has_value(); }
};
