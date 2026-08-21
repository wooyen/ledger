#include "ledger/core/rational.hpp"
#include <numeric>
#include <stdexcept>
#include <charconv>
#include <sstream>
using namespace std;

namespace ledger {

const Rational Rational::NaN{0, UINT64_C(0)};
const Rational Rational::POSINF{1, UINT64_C(0)};
const Rational Rational::NEGINF{-1, UINT64_C(0)};

namespace {

template<typename T>T sign(T val) {
    return val < 0 ? T(-1) : (val == T(0) ? T(0) : T(1));
}

template<typename T> requires is_signed_v<T> make_unsigned_t<T> safe_abs(T n) {
	return n == numeric_limits<T>::min() ? static_cast<make_unsigned_t<T>>(numeric_limits<T>::max()) + 1 : static_cast<make_unsigned_t<T>>(abs(n));
}

template<typename T> requires is_unsigned_v<T> T safe_abs(T n) {
	return n;
}

void reduce(int64_t &n, uint64_t &d) {
	if (d == 0) {
		n = sign(n);
		return;
	}
	auto g = gcd(safe_abs(n), d);
	n /= static_cast<int64_t>(g);
	d /= g;
}

using wide = __int128;
const int format_idx = ios_base::xalloc();

struct ShowPos {
	ostream &os;
	bool was_showpos;
	ShowPos(ostream &os, bool show = false) : os(os) {
		was_showpos = os.flags() & ios_base::showpos;
		if (show) {
			os << showpos;
		} else {
			os << noshowpos;
		}
	}
	~ShowPos() {
		if (was_showpos) {
			os << showpos;
		} else {
			os << noshowpos;
		}
	}
};
bool nan_calc(Rational &lhs, Rational const &rhs) {
	if (rhs.is_nan()) {
		lhs = Rational::NaN;
		return true;
	}
	return lhs.is_nan();
}
}

Rational::Rational(int64_t n, uint64_t d) : _num(n), _den(d) {
	reduce(_num, _den);
}

Rational::Rational(int64_t n, int64_t d) : _num(n), _den(safe_abs(d)) {
	reduce(_num, _den);
	if (d < 0) _num = -_num;
}

partial_ordering Rational::operator<=>(Rational const& rhs) const {
	if (is_nan() || rhs.is_nan()) return std::partial_ordering::unordered;
    // 交叉相乘比较：num/denom vs rhs.num/rhs.denom
    // 使用 __int128 防止溢出
    using wide = __int128;
    auto lhs_val = static_cast<wide>(_num) * static_cast<wide>(rhs._den);
    auto rhs_val = static_cast<wide>(rhs._num) * static_cast<wide>(rhs._den);
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
    reduce(_num, _den);
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
    reduce(_num, _den);
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
	reduce(_num, _den);
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

istream &operator>>(istream &is, Rational &r) {
	int64_t x;
	if (!(is >> x)) {
		return is;
	}
	char c = is.peek();
	if (c == '/') {
		is.get();
		uint64_t y;
		if (is >> y) {
			r = Rational(x, y);
			return is;
		} else {
			r = x;
			is.clear(is.failbit);
			is.unget();
			return is;
		}
	} else if (c == '+' || c == '-') {
		is.get();
		is.unget();
		int64_t y;
		if (!(is>>y)) {
			r = x;
			is.clear(is.failbit);
			return is;
		}
		char c2 = is.peek();
		if (c2 == '/') {
			is.get();
			uint64_t z;
			if (is >> z) {
				r = Rational(y, z) + x;
				return is;
			} else {
				r = Rational(x + y);
				is.clear(is.failbit);
				is.unget();
				return is;
			}
		}
	} else if (c == '.') {
		is.get();
		char c2;
		int64_t e = 0;
		uint64_t d = 1;
		while (true) {
			c2 = is.peek();
			if (c2 >= '0' && c2 <= '9') {
				is.get();
				x = x * 10 + (c2 - '0');
				--e;
			} else if (c2 == 'e' || c2 == 'E') {
				is.get();
				int64_t exp;
				if (!(is >> exp)) {
					r = Rational(x, static_cast<uint64_t>(d));
					is.clear(is.failbit);
					is.unget();
					return is;
				}
				e += exp;
				break;
			} else {
				break;
			}
		}
		while (e > 0) { x *= 10; --e; }
		while (e < 0) { d *= 10; ++e; }
		r = Rational(x, d);
		return is;
	}
	r = x;
	return is;
}

ostream &operator<<(ostream &os, Rational const &r) {
	if (r.is_nan()) return os << "NaN";
	if (r.is_inf()) return os << (r.is_positive_inf() ? "INF" : "-INF");
	if (r.is_zero()) return os << "0";
	if (r._den == 1 && os.iword(format_idx) != static_cast<int>(Rational::Format::DECIMAL))
			return os << r._num;
	switch (os.iword(format_idx)) {
	case static_cast<int>(Rational::Format::MIXED): {
		auto i = r._num / r._den;
		auto n = r._num % r._den;
		os << i;
		ShowPos showpos(os, true);
		os << n << "/" << r._den;
		break;
	}
	case static_cast<int>(Rational::Format::EXACT):
		os << r._num << "/" << r._den;
		break;
	case static_cast<int>(Rational::Format::DECIMAL):
	default:
		os << static_cast<long double>(static_cast<__float128>(r._num) / r._den);
		break;
	}
	return os;
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
