#pragma once

#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct BlockedClient {
    int fd;
    std::string key;
    std::chrono::steady_clock::time_point deadline;

    bool is_indefinite() const { return deadline == std::chrono::steady_clock::time_point::max(); }
};

class BlockingManager {
    std::unordered_map<std::string, std::deque<BlockedClient>> blocked_clients_;
    std::unordered_map<int, BlockedClient*> fd_to_client_;
  public:
    void block_client(int fd, std::string key, std::chrono::milliseconds timeout);
    std::optional<BlockedClient> wake_client(const std::string& key);
    std::vector<int> get_expired_clients();
    void unblock_client(int fd);
    std::optional<std::chrono::steady_clock::time_point> get_next_deadline() const;
    bool is_blocked(int fd) const;
    size_t blocked_count() const;
};
