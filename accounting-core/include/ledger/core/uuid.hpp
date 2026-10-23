#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <compare>
#include <format>
#include "ledger/core/util.hpp"

namespace ledger {

struct Uuid {
    std::array<uint8_t, 16> bytes{};

    static Uuid generate();
    static Uuid from_string(std::string_view);
    std::string to_string(bool upper = true) const;

    bool operator==(Uuid const&) const = default;
    std::strong_ordering operator<=>(Uuid const&) const = default;

    bool is_null() const;

};

} // namespace ledger

// std::hash 特化（用于 unordered_map）
template <> struct std::hash<ledger::Uuid> {
    auto operator()(ledger::Uuid const& u) const -> size_t;
};

template<> class formatter<ledger::Uuid> {
	char _type = 'L';
public:
    constexpr auto parse(format_parse_context& ctx) const {
		auto it = ctx.begin();
		auto end = ctx.end();
		if (it != end) {
			switch (*it) {
			case 'L':
			case 'l':
			case 'S':
			case 's':
				_type = *it++;
			default:
				throw format_error("invalid format");
			}
		}
		if (it != end && *it != '}') throw format_error("invalid format");
		return it;
	}
    auto format(const ledger::Uuid& u, format_context& ctx) const) {
		static constexpr auto hex_upper = "0123456789ABCDEF";
		static constexpr auto hex_lower = "0123456789abcdef";
		auto out = ctx.out();
		for (int i = 0; i < 16; ++i) {
			if ((_type == 'S' || _type == 's') && (i == 4 || i == 6 || i == 8 || i == 10)) *out++ = '-';
			if (_type == 'L' || _type == 'S') {
				*out++ = __internal::hex_upper[u.bytes[i] >> 4];
				*out++ = __internal::hex_upper[u.bytes[i] & 0x0f];
			} else if (_type == 'l' || _type == 's') {
				*out++ = __internal::hex_lower[u.bytes[i] >> 4];
				*out++ = __internal::hex_lower[u.bytes[i] & 0x0f];
			}
		}
		return out;
	}
};
