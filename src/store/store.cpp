#include "store.hpp"

std::chrono::steady_clock::time_point Store::get_current_time() {
    return std::chrono::steady_clock::now();
}

bool Store::is_expired(const Entry& entry) const {
    return entry.expiry && get_current_time() >= *entry.expiry;
}

void Store::set(const std::string& key, const std::string& value, std::optional<uint64_t> ttl_ms) {
    Entry entry;
    entry.value = value;
    if (ttl_ms) {
        entry.expiry = get_current_time() + std::chrono::milliseconds(*ttl_ms);
    }
    data_[key] = std::move(entry);
}

std::optional<std::string> Store::get(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return std::nullopt;
    }

    Entry& entry = it->second;
    if (is_expired(entry)) {
        data_.erase(it);
        return std::nullopt;
    }

    if (std::holds_alternative<std::string>(entry.value)) {
        return std::get<std::string>(entry.value);
    }
    return std::nullopt;
}

bool Store::exists(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return false;
    }

    if (is_expired(it->second)) {
        data_.erase(it);
        return false;
    }

    return true;
}

bool Store::del(const std::string& key) { return data_.erase(key) > 0; }

int64_t Store::rpush(const std::string& key, const std::string& value) {
    auto it = data_.find(key);

    if (it == data_.end() || is_expired(it->second)) {
        if (it != data_.end()) {
            data_.erase(it);
        }
        Entry entry;
        entry.value = std::deque<std::string>{value};
        data_[key] = std::move(entry);
        return 1;
    }

    Entry& entry = it->second;
    if (!std::holds_alternative<std::deque<std::string>>(entry.value)) {
        entry.value = std::deque<std::string>{};
    }

    auto& list = std::get<std::deque<std::string>>(entry.value);
    list.push_back(value);
    return static_cast<int64_t>(list.size());
}
