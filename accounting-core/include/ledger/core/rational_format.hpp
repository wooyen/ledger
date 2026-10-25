#pragma once
#include <cctype>
#include <format>
#include <string>
#include <string_view>
#include <algorithm>
#include <array>
#include <limits>
#include <bit>
#include <iterator>
#include "rational.hpp"
#include "ledger/core/util.hpp"

namespace std {

template <>struct formatter<ledger::Rational> {
private:
	enum class align_t { left, right, center };

	char _mode = 'm'; // 'd' decimal, 'm' mixed, 'f' fraction

	// -- (m / f mode) --
	char _pad_fill  = ' ';
	align_t _pad_align = align_t::right;
	size_t _pad_width = 0;
	int _pad_width_arg = -1;
	char _sign = '-';
	bool _prefix = false;
	char _type = 'd';

	formatter<long double> _float_fmt;
	formatter<uint64_t> _uint64_fmt;

	bool uppercase() const {
		return _type >= 'A' && _type <= 'Z';
	}
	// ── d: decimal ──────────────────────────────────────
	template<typename FormatContext> auto format_decimal(const ledger::Rational& r, FormatContext& ctx) const {
		if (r.is_nan()) {
			return _float_fmt.format(numeric_limits<long double>::quiet_NaN(), ctx);
		} else if (r.is_positive_inf()) {
			return _float_fmt.format(numeric_limits<long double>::infinity(), ctx);
		} else if (r.is_negative_inf()) {
			return _float_fmt.format(-numeric_limits<long double>::infinity(), ctx);
		}
		auto val = static_cast<__float128>(r.numerator()) / r.denominator();
		return _float_fmt.format(static_cast<long double>(val), ctx);
	}

	template<typename FormatContext> pair<size_t, size_t> padding_size(size_t width, FormatContext& ctx) const {
		size_t pad_width = _pad_width;
		if (_pad_width_arg >= 0) {
			bool ok = false;
			std::visit_format_arg([&pad_width, &ok](auto v) {
				using T = std::remove_cvref_t<decltype(v)>;
				if constexpr (std::integral<T>) {
					pad_width = static_cast<size_t>(v);
					ok = true;
				}
			}, ctx.arg(_pad_width_arg));
			if (!ok) {
				throw format_error("width argument is not an integer");
			}
		}
		size_t left = 0;
		size_t right = 0;
		if (width < pad_width) {
			switch (_pad_align) {
			case align_t::left:
				right = pad_width - width;
				break;
			case align_t::right:
				left = pad_width - width;
				break;
			case align_t::center:
				left = (pad_width - width) / 2;
				right = pad_width - width - left;
				break;
			}
		}
		return {left, right};
	}

	// apply outer width / fill / alignment to a composed string
	template<typename Iter, typename FormatContext>auto apply_padding(Iter beg, Iter end, FormatContext &ctx) const {
		auto [left, right] = padding_size(distance(beg, end), ctx);
		auto out = ctx.out();
		out = fill_n(out, left, _pad_fill);
		out = copy(beg, end, out);
		out = fill_n(out, right, _pad_fill);
		return out;

	}

	template<typename FormatContext> auto format_special(const ledger::Rational& r, FormatContext& ctx) const {
		auto str = r.is_nan() ? array<char, 4>{_sign, 'n', 'a', 'n'} : array<char, 4>{_sign, 'i', 'n', 'f'};
		if (r.numerator() < 0) {
			str[0] = '-';
		}
		if (isupper(_type)) {
			for (int i = 1; i < 4; ++i) {
				str[i] = toupper(str[i]);
			}
		}
		return apply_padding(str.begin() + (_sign == '-' && r.numerator() >= 0 ? 1 : 0), str.end(), ctx);
	}

	template<typename T> requires is_unsigned_v<T> size_t number_width(T n) const {
		if (n == 0) {
			return !_prefix || _type == 'd' || _type == 'D' || _type == 'o' || _type == 'O' ? 1 : 3;
		}
		static constexpr auto powers10 = [] {
			std::array<T, numeric_limits<T>::digits10 + 1> powers;
			powers[0] = 1;
			for (size_t i = 1; i < powers.size(); ++i) {
				powers[i] = powers[i - 1] * 10;
			}
			return powers;
		}();
		if (_type == 'd' || _type == 'D') {
			return std::ranges::upper_bound(powers10, n) - powers10.begin();
		}
		size_t width = numeric_limits<T>::digits - std::countl_zero(n);
		if (_type == 'b' || _type == 'B') {
			return width + (_prefix ? 2 : 0);
		} else if (_type == 'o' || _type == 'O') {
			return (width + 2) / 3 + (_prefix ? 1 : 0);
		} else if (_type == 'x' || _type == 'X') {
			return (width + 3) / 4 + (_prefix ? 2 : 0);
		} else {
			throw std::invalid_argument("invalid base");
		}
	}

	template<typename Iter> auto add_sign(int64_t n, Iter& out) const {
		if (n < 0) {
			*out++ = '-';
		} else if (_sign != '-') {
			*out++ = _sign;
		}
		return out;
	}
	template<typename FormatContext> auto format_int(int64_t n, FormatContext& ctx) const {
		auto pn = ledger::__internal::abs(n);
		size_t width = number_width(pn) + (n < 0 || _sign != '-' ? 1 : 0);
		auto [left, right] = padding_size(width, ctx);
		auto out = fill_n(ctx.out(), left, _pad_fill);
		ctx.advance_to(add_sign(n, out));
		out = _uint64_fmt.format(pn, ctx);
		return fill_n(out, right, _pad_fill);
	}
	// ── m: mixed fraction  "2 1/3" ──────────────────────
	template<typename FormatContext> auto format_mixed(const ledger::Rational& r, FormatContext& ctx) const {
		auto den = r.denominator();
		auto num = ledger::__internal::abs(r.numerator());
		auto int_part = num / den;
		num %= den;
		if (int_part == 0) {
			return format_fraction(r, ctx);
		}
		auto width = number_width(int_part) + number_width(num) + number_width(den) + 2;
		if (r.numerator() < 0 || _sign != '-') {
			++width;
		}
		auto [left, right] = padding_size(width, ctx);
		auto out = fill_n(ctx.out(), left, _pad_fill);
		ctx.advance_to(add_sign(r.numerator(), out));
		out = _uint64_fmt.format(int_part, ctx);
		*out++ = '&';
		ctx.advance_to(out);
		out = _uint64_fmt.format(num, ctx);
		*out++ = '/';
		ctx.advance_to(out);
		out = _uint64_fmt.format(den, ctx);
		return fill_n(out, right, _pad_fill);
	}

	// ── f: exact fraction  "7/3" ────────────────────────
	template<typename FormatContext> auto format_fraction(const ledger::Rational& r, FormatContext& ctx) const {
		auto num = ledger::__internal::abs(r.numerator());
		auto den = r.denominator();
		auto width = number_width(num) + number_width(den) + 1;
		if (r.numerator() < 0 || _sign != '-') {
			++width;
		}
		auto [left, right] = padding_size(width, ctx);
		auto out = fill_n(ctx.out(), left, _pad_fill);
		ctx.advance_to(add_sign(r.numerator(), out));
		out = _uint64_fmt.format(num, ctx);
		*out++ = '/';
		ctx.advance_to(out);
		out = _uint64_fmt.format(den, ctx);
		return fill_n(out, right, _pad_fill);
	}

public:
	constexpr auto parse(format_parse_context& ctx) {
		auto it  = ctx.begin();
		auto end = ctx.end();
		if (it == end) return it;
		if (*it == 'd' || *it == 'm' || *it == 'f') {
			_mode = *it++;
		}
		if (it == end || *it == '}') return it;
		// ── d mode: delegate the *entire* remaining spec to std::formatter<long double> ──
		ctx.advance_to(it);
		if (_mode == 'd') {
			return _float_fmt.parse(ctx);
		}
		if (it == end || *it == '}') return it;

		// ── m / f mode: [fill][align][sign][width]['#']['L'][type]
		// [fill][align]
		if (*it == '<') {
			_pad_align = align_t::left;
			++it;
		} else if (*it == '>') {
			_pad_align = align_t::right;
			++it;
		} else if (*it == '^') {
			_pad_align = align_t::center;
			++it;
		} else if (auto nxt = it + 1; nxt != end) {
			if (*nxt == '<') {
				_pad_fill = *it;
				_pad_align = align_t::left;
				it += 2;
			} else if (*nxt == '>') {
				_pad_fill = *it;
				_pad_align = align_t::right;
				it += 2;
			} else if (*nxt == '^') {
				_pad_fill = *it;
				_pad_align = align_t::center;
				it += 2;
			}
		}
		if (it == end || *it == '}') return it;

		// [sign]
		if (*it == '+') {
			_sign = '+';
			++it;
		} else if (*it == ' ') {
			_sign = ' ';
			++it;
		} else if (*it == '-') {
			_sign = '-';
			++it;
		}
		if (it == end || *it == '}') return it;

		// [width]
		if (*it >= '1' && *it <= '9') {
			_pad_width = 0;
			while (it != end && *it >= '0' && *it <= '9') {
				_pad_width = _pad_width * 10 + (*it++ - '0');
			}
		} else if (*it == '{' && it + 1 != end && *(it + 1) == '}') {
			_pad_width_arg = static_cast<int>(ctx.next_arg_id());
			it += 2;
		}

		// ── delegates the remaining characters to formatter<uint64_t>.
		// but we need to check it is in the form of "['#'][type]"
		array<char, 2> spec;
		auto pos = spec.begin();
		if (it != end && *it == '#') {
			_prefix = true;
			*pos++ = *it++;
		}
		if (it != end && *it != '}') {
			_type = *it++;
			*pos++ = _type == 'O' || _type == 'D' ? _type + ('a' - 'A') : _type;
		}
		if (it != end && *it != '}') {
			throw format_error("invalid format specifier");
		}
		string_view sv(spec.data(), static_cast<size_t>(pos - spec.begin()));
		format_parse_context sub_ctx(sv);
		_uint64_fmt.parse(sub_ctx);
		return it;
	}

	template<typename FormatContext> auto format(const ledger::Rational &r, FormatContext &ctx) const {
		if (_mode == 'd') {
			return format_decimal(r, ctx);
		} else if (r.denominator() == 0) {
			return format_special(r, ctx);
		} else if (r.denominator() == 1) {
			return format_int(r.numerator(), ctx);
		} else if (_mode == 'm') {
			return format_mixed(r, ctx);
		} else {
			return format_fraction(r, ctx);
		}
	}
};

} // namespace std
