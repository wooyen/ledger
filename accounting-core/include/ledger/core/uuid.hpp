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

template<> class std::formatter<ledger::Uuid> {
	char _type = 'L';
public:
    constexpr auto parse(std::format_parse_context& ctx) {
		auto it = ctx.begin();
		auto end = ctx.end();
		if (it != end && *it != '}') {
			switch (*it) {
			case 'L':
			case 'l':
			case 'S':
			case 's':
				_type = *it++;
				break;
			default:
				throw std::format_error("invalid format");
			}
			if (it != end && *it != '}') throw std::format_error("invalid format");
		}
		return it;
	}
    auto format(const ledger::Uuid& u, std::format_context& ctx) const {
		auto out = ctx.out();
		for (int i = 0; i < 16; ++i) {
			if ((_type == 'S' || _type == 's') && (i == 4 || i == 6 || i == 8 || i == 10)) *out++ = '-';
			*out++ = ledger::__internal::hex_char(u.bytes[i] >> 4, _type == 'L' || _type == 'S');
			*out++ = ledger::__internal::hex_char(u.bytes[i] & 0x0F, _type == 'L' || _type == 'S');
		}
		return out;
	}
};
