#include "store.hpp"

std::chrono::steady_clock::time_point Store::get_current_time() {
    return std::chrono::steady_clock::now();
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

    const Entry& entry = it->second;
    if (entry.expiry && get_current_time() >= *entry.expiry) {
        data_.erase(it);
        return std::nullopt;
    }

    return entry.value;
}

bool Store::exists(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return false;
    }

    const Entry& entry = it->second;
    if (entry.expiry && get_current_time() >= *entry.expiry) {
        data_.erase(it);
        return false;
    }

    return true;
}

bool Store::del(const std::string& key) { return data_.erase(key) > 0; }
