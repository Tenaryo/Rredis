#include "blocking_manager.hpp"
#include <algorithm>

bool BlockingManager::parse_entry_id(const std::string& id, int64_t& timestamp, int64_t& sequence) {
    auto dash_pos = id.find('-');
    if (dash_pos == std::string::npos) {
        return false;
    }

    try {
        size_t pos;
        timestamp = std::stoll(id.substr(0, dash_pos), &pos);
        if (pos != dash_pos) {
            return false;
        }
        sequence = std::stoll(id.substr(dash_pos + 1), &pos);
        if (pos != id.length() - dash_pos - 1) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool BlockingManager::compare_entry_id(const std::string& a, const std::string& b) {
    int64_t ts_a, seq_a, ts_b, seq_b;
    if (!parse_entry_id(a, ts_a, seq_a) || !parse_entry_id(b, ts_b, seq_b)) {
        return false;
    }
    if (ts_a != ts_b) {
        return ts_a < ts_b;
    }
    return seq_a < seq_b;
}

void BlockingManager::block_client(int fd, std::string key, std::chrono::milliseconds timeout) {
    auto deadline = timeout.count() == 0 ? std::chrono::steady_clock::time_point::max()
                                         : std::chrono::steady_clock::now() + timeout;

    BlockedClient client{fd, std::move(key), deadline, ""};
    blocked_clients_[client.key].push_back(client);
    fd_to_client_[fd] = &blocked_clients_[client.key].back();
}

void BlockingManager::block_client_for_stream(int fd,
                                              std::string key,
                                              std::string last_id,
                                              std::chrono::milliseconds timeout) {
    auto deadline = timeout.count() == 0 ? std::chrono::steady_clock::time_point::max()
                                         : std::chrono::steady_clock::now() + timeout;

    BlockedClient client{fd, std::move(key), deadline, std::move(last_id)};
    blocked_clients_[client.key].push_back(client);
    fd_to_client_[fd] = &blocked_clients_[client.key].back();
}

std::optional<BlockedClient> BlockingManager::wake_client(const std::string& key) {
    auto it = blocked_clients_.find(key);
    if (it == blocked_clients_.end() || it->second.empty()) {
        return std::nullopt;
    }

    BlockedClient client = std::move(it->second.front());
    it->second.pop_front();
    fd_to_client_.erase(client.fd);

    if (it->second.empty()) {
        blocked_clients_.erase(it);
    }

    return client;
}

std::optional<BlockedClient>
BlockingManager::wake_client_for_stream(const std::string& key, const std::string& new_entry_id) {
    auto it = blocked_clients_.find(key);
    if (it == blocked_clients_.end() || it->second.empty()) {
        return std::nullopt;
    }

    for (auto client_it = it->second.begin(); client_it != it->second.end(); ++client_it) {
        if (compare_entry_id(client_it->last_id, new_entry_id)) {
            BlockedClient client = std::move(*client_it);
            it->second.erase(client_it);
            fd_to_client_.erase(client.fd);

            if (it->second.empty()) {
                blocked_clients_.erase(it);
            }

            return client;
        }
    }

    return std::nullopt;
}

std::vector<int> BlockingManager::get_expired_clients() {
    std::vector<int> expired;
    auto now = std::chrono::steady_clock::now();

    for (auto& [key, queue] : blocked_clients_) {
        while (!queue.empty()) {
            auto& client = queue.front();
            if (!client.is_indefinite() && client.deadline <= now) {
                expired.push_back(client.fd);
                fd_to_client_.erase(client.fd);
                queue.pop_front();
            } else {
                break;
            }
        }
    }

    std::erase_if(blocked_clients_, [](const auto& p) { return p.second.empty(); });

    return expired;
}

void BlockingManager::unblock_client(int fd) {
    auto it = fd_to_client_.find(fd);
    if (it == fd_to_client_.end()) {
        return;
    }

    BlockedClient* client = it->second;
    std::string key = client->key;

    auto queue_it = blocked_clients_.find(key);
    if (queue_it != blocked_clients_.end()) {
        std::erase_if(queue_it->second, [fd](const BlockedClient& c) { return c.fd == fd; });
        if (queue_it->second.empty()) {
            blocked_clients_.erase(queue_it);
        }
    }

    fd_to_client_.erase(it);
}

std::optional<std::chrono::steady_clock::time_point> BlockingManager::get_next_deadline() const {
    if (blocked_clients_.empty()) {
        return std::nullopt;
    }

    std::optional<std::chrono::steady_clock::time_point> earliest;
    for (const auto& [key, queue] : blocked_clients_) {
        if (!queue.empty() && !queue.front().is_indefinite()) {
            auto deadline = queue.front().deadline;
            if (!earliest || deadline < *earliest) {
                earliest = deadline;
            }
        }
    }

    return earliest;
}

bool BlockingManager::is_blocked(int fd) const { return fd_to_client_.contains(fd); }

size_t BlockingManager::blocked_count() const { return fd_to_client_.size(); }
