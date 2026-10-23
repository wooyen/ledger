#include <limits>
#include <numeric>
#include <istream>
#include "ledger/core/rational.hpp"
#include "ledger/core/rational_format.hpp"
using namespace std;

namespace ledger {

const Rational Rational::ZERO{};
const Rational Rational::ONE{1};
const Rational Rational::NaN{0, UINT64_C(0)};
const Rational Rational::POSINF{1, UINT64_C(0)};
const Rational Rational::NEGINF{-1, UINT64_C(0)};

	namespace {

using wide = __int128;
const int format_idx = ios_base::xalloc();

bool nan_calc(Rational &lhs, Rational const &rhs) {
	if (rhs.is_nan()) {
		lhs = Rational::NaN;
		return true;
	}
	return lhs.is_nan();
}
	}
void Rational::reduce() {
	if (_den == 0) {
		_num = __internal::sign(_num);
		return;
	}
	auto g = gcd(__internal::abs(_num), _den);
	_num /= static_cast<int64_t>(g);
	_den /= g;
}

partial_ordering Rational::operator<=>(Rational const& rhs) const {
	if (is_nan() || rhs.is_nan()) return std::partial_ordering::unordered;
    using wide = __int128;
    auto lhs_val = static_cast<wide>(_num) * static_cast<wide>(rhs._den);
    auto rhs_val = static_cast<wide>(rhs._num) * static_cast<wide>(_den);
    if (lhs_val < rhs_val) return std::partial_ordering::less;
    if (lhs_val > rhs_val) return std::partial_ordering::greater;
    return std::partial_ordering::equivalent;
}

Rational& Rational::operator+=(Rational const& rhs) {
	if (nan_calc(*this, rhs)) return *this;
	if (is_inf() && rhs.is_inf()) {
		if (_num != rhs._num) *this = Rational::NaN;
		return *this;
	}
	if (is_inf()) return *this;
	if (rhs.is_inf()) {
		*this = rhs;
		return *this;
	}
    // a/b + c/d = (a*d + c*b) / (b*d)，然后约分
	auto g = gcd(_den, rhs._den);
    auto n = static_cast<wide>(_num) * static_cast<wide>(rhs._den/g)
           + static_cast<wide>(rhs._num) * static_cast<wide>(_den/g);
    auto d = static_cast<wide>(_den) * static_cast<wide>(rhs._den/g);
    _num = static_cast<int64_t>(n);
    _den = static_cast<uint64_t>(d);
    reduce();
    return *this;
}

Rational& Rational::operator-=(Rational const& rhs) {
	if (nan_calc(*this, rhs)) return *this;
	if (is_inf() && rhs.is_inf()) {
		if (_num == rhs._num) *this = Rational::NaN;
		return *this;
	}
	if (is_inf()) return *this;
	if (rhs.is_inf()) {
		*this = -rhs;
		return *this;
	}
	auto g = gcd(_den, rhs._den);
    auto n = static_cast<wide>(_num) * static_cast<wide>(rhs._den/g)
           - static_cast<wide>(rhs._num) * static_cast<wide>(_den/g);
    auto d = static_cast<wide>(_den) * static_cast<wide>(rhs._den/g);
    _num = static_cast<int64_t>(n);
    _den = static_cast<uint64_t>(d);
    reduce();
    return *this;
}

Rational& Rational::operator*=(Rational const &rhs) {
	if (nan_calc(*this, rhs)) return *this;
	if (is_inf()) {
		if (rhs.is_zero()) *this = Rational::NaN;
		return *this;
	}
	if (rhs.is_inf()) {
		if (is_zero()) *this = Rational::NaN;
		else *this = rhs;
		return *this;
	}
	auto g1 = gcd(_num, rhs._den);
	auto g2 = gcd(_den, rhs._num);
	_num = (_num / g1) * (rhs._num / g2);
	_den = (_den / g2) * (rhs._den / g1);
	reduce();
	return *this;
}

Rational& Rational::operator/=(Rational const &rhs) {
	if (nan_calc(*this, rhs)) return *this;
	if (rhs.is_zero()) {
		_den = 0;
		return *this;
	}
    if (rhs.is_inf()) {
		if (is_inf()) {
			*this = Rational::NaN;
		} else {
			*this = Rational();
		}
		return *this;
	}
	if (is_inf()) return *this;
	auto g1 = gcd(_num, rhs._num);
	auto g2 = gcd(_den, rhs._den);
	_num = (_num/g1) * (rhs._den/g2);
	_den = (_den/g2) * (rhs._num/g1);
    return *this;
}

int parse_digit(char c, int base) {
	int n = c - '0';
	if (n >= 0 && n < base) return n;
	if (base == 16) {
		n = c >= 'a' ? c - 'a' + 10 : c - 'A' + 10;
		if (n >= 10 && n < base) return n;
	}
	return -1;
}

istream &parse_decimal(istream &is, Rational &r, bool neg, int64_t x = 0) {
	is.get();
	int64_t e = 0;
	uint64_t d = 1;
	int base;
	switch (is.flags() & ios_base::basefield) {
	case ios_base::oct:
		base = 8;
		break;
	case ios_base::hex:
		base = 16;
		break;
	default:
		base = 10;
	}
	char c = is.peek();
	int n = 0;
	while ((n = parse_digit(c, base)) != -1) {
		x = x * base + (neg ? - n : n);
		d *= base;
		is.get();
		c = is.peek();
	}
	if (c == 'e' || c == 'E' || (base == 16 && (c == 'p' || c == 'P'))) {
		is.get();
		c = is.peek();
		if (c == ' ' || (c >= 9 && c < 14) || !(is >> e)) {
			is.setstate(ios_base::failbit);
			return is;
		}
	}
	while (e < 0) { d *= base; ++e; }
	while (e > 0 && d > 1) { d /= base; --e; }
	while (e > 0) { x *= base; --e; }
	r = Rational(x, d);
	return is;
}

bool ensure_number_available(istream &is) {
	is.get();
	int c = is.peek();
	if (!(c >= '0' && c <= '9')) {
		is.setstate(ios_base::failbit);
		return false;
	}
	return true;
}

istream &operator>>(istream &is, Rational &r) {
	if (!istream::sentry(is)) return is;
	int first = is.peek();
	if (first == istream::traits_type::eof()) return is;
	bool neg = first == '-';
	r = Rational::ZERO;
	if (first == '.') return parse_decimal(is, r, neg);
	int64_t x;
	if (!(is >> x)) {
		is.clear();
		array<const char *, 4> symbol{"nan", "inf", "NAN", "INF"};
		int c2 = is.peek();
		if (c2 == '.') return parse_decimal(is, r, neg);
		for (int i = 0; i < 2; ++i) {
			int j = 0;
			for (; j < 3; ++j) {
				if (c2 != symbol[i][j] && c2 != symbol[i + 2][j]) break;
				is.get();
				if (j < 2) c2 = is.peek();
			}
			if (j == 3) {
				r = i == 0 ? Rational::NaN : (neg ? Rational::NEGINF : Rational::POSINF);
				return is;
			} else if (j != 0) {
				is.setstate(ios_base::failbit);
				return is;
			}
		}
		is.setstate(ios_base::failbit);
		return is;
	}
	if (is.eof()) {
		r = x;
		return is;
	}
	int delim = is.peek();
	if (delim == '/') {
		if (!ensure_number_available(is)) return is;
		uint64_t y;
		if (!(is >> y)) return is;
		r = Rational(x, y);
		return is;
	} else if (delim == '&') {
		if (!ensure_number_available(is)) return is;
		uint64_t y;
		if (!(is >> y)) return is;
		if (is.peek() != '/') {
			is.setstate(ios_base::failbit);
			return is;
		}
		if (!ensure_number_available(is)) return is;
		uint64_t z;
		if (!(is >> z)) return is;
		r = Rational(y, z);
		if (neg) r = -r;
		r += x;
		return is;
	} else if (delim == '.') {
		return parse_decimal(is, r, neg, x);
	}
	r = x;
	return is;
}

ostream &operator<<(ostream &os, Rational const &r) {
	Rational::Format format = static_cast<Rational::Format>(os.iword(format_idx));
	if (format == Rational::Format::DECIMAL) {
		if (r.is_nan()) return os << numeric_limits<long double>::quiet_NaN();
		if (r.is_positive_inf()) return os << numeric_limits<long double>::infinity();
		if (r.is_negative_inf()) return os << -numeric_limits<long double>::infinity();
		return os << static_cast<long double>(static_cast<__float128>(r._num) / r._den);
	} else if (r._den == 1) {
		return os << r._num;
	} else {
		array<char, 8> spec;
		auto pos = spec.begin();
		*pos++ = '{';
		*pos++ = ':';
		*pos++ = format == Rational::Format::MIXED ? 'm' : 'f';
		if (os.flags() & ios_base::showpos) {
			*pos++ = '+';
		}
		if (os.flags() & ios_base::showbase) {
			*pos++ = '#';
		}
		char type = 'd';
		switch (os.flags() & ios_base::basefield) {
		case ios_base::oct:
			type = 'o';
			break;
		case ios_base::hex:
			type = 'x';
			break;
		}
		if (os.flags() & ios_base::uppercase) {
			type = toupper(type);
		}
		if (type != 'd') {
			*pos++ = type;
		}
		*pos++ = '}';
		string_view sv(spec.data(), static_cast<size_t>(pos - spec.begin()));
		return os << vformat(sv, make_format_args(r));
	}
}

ostream &decimal(ostream &os) {
    os.iword(format_idx) = static_cast<int>(Rational::Format::DECIMAL);
    return os;
}

ostream &mixed(ostream &os) {
    os.iword(format_idx) = static_cast<int>(Rational::Format::MIXED);
    return os;
}

ostream &exact(ostream &os) {
    os.iword(format_idx) = static_cast<int>(Rational::Format::EXACT);
    return os;
}

} // namespace ledger
