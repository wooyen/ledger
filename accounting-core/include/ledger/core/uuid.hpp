#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <compare>

namespace ledger {

struct Uuid {
    std::array<uint8_t, 16> bytes{};

    static Uuid generate();
    static Uuid from_string(std::string_view);
    std::string to_string() const;

    bool operator==(Uuid const&) const = default;
    std::strong_ordering operator<=>(Uuid const&) const = default;

    bool is_null() const;

};

} // namespace ledger

// std::hash 特化（用于 unordered_map）
template <> struct std::hash<ledger::Uuid> {
    auto operator()(ledger::Uuid const& u) const -> size_t;
};
