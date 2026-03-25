#include "store.hpp"

void Store::set(const std::string& key, const std::string& value) { data_[key] = value; }

std::optional<std::string> Store::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool Store::exists(const std::string& key) const { return data_.contains(key); }

bool Store::del(const std::string& key) { return data_.erase(key) > 0; }
