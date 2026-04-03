#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct StreamId {
    int64_t timestamp{};
    int64_t sequence{};

    static std::optional<StreamId> parse(std::string_view id) {
        auto dash = id.find('-');
        if (dash == std::string_view::npos)
            return std::nullopt;

        int64_t ts{};
        auto [ptr1, ec1] = std::from_chars(id.data(), id.data() + dash, ts);
        if (ec1 != std::errc{} || ptr1 != id.data() + dash)
            return std::nullopt;

        int64_t seq{};
        auto [ptr2, ec2] = std::from_chars(id.data() + dash + 1, id.data() + id.size(), seq);
        if (ec2 != std::errc{} || ptr2 != id.data() + id.size())
            return std::nullopt;

        return StreamId{ts, seq};
    }

    std::string to_string() const {
        return std::to_string(timestamp) + "-" + std::to_string(sequence);
    }

    auto operator<=>(const StreamId&) const = default;
};
