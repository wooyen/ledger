#include "ledger/core/rational.hpp"
#include <numeric>
#include <stdexcept>
#include <charconv>
#include <sstream>
#include <cmath>
using namespace std;

namespace ledger {
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

Rational Rational::operator+(Rational const& rhs) const {
	if (is_nan() || rhs.is_nan()) return Rational::NAN;
	if (is_inf() && rhs.is_inf()) return _num == rhs._num ? rhs : Rational::NAN;
	if (is_inf()) return *this;
	if (rhs.is_inf()) return rhs;
    // a/b + c/d = (a*d + c*b) / (b*d)，然后约分
    using wide = __int128;
	auto g = gcd(_den, rhs._den);
    auto n = static_cast<wide>(_num) * static_cast<wide>(rhs._den/g)
           + static_cast<wide>(rhs._num) * static_cast<wide>(_den/g);
    auto d = static_cast<wide>(_den) * static_cast<wide>(rhs._den/g);

    return Rational{static_cast<int64_t>(n), static_cast<int64_t>(d)};
}

Rational Rational::operator*(Rational const &rhs) const {
	if (is_nan() || rhs.is_nan()) return Rational::NAN;
	if (is_inf()) return rhs.is_zero() ? Rational::NAN : *this;
	if (rhs.is_inf()) return is_zero() ? Rational::NAN : rhs;
	auto g1 = gcd(_num, rhs._den);
	auto g2 = gcd(_den, rhs._num);
	return Rational{(_num/g1) * (rhs._num/g2), (_den/g2) * (rhs._den/g1)};
}

Rational Rational::operator/(Rational const &rhs) const {
	if (is_nan() || rhs.is_nan()) return Rational::NAN;
    if (rhs.is_zero()) return Rational(_num, 0);
    if (rhs.is_inf()) return is_inf() ? Rational::NAN : Rational();
	if (is_inf()) return *this;
    return *this * Rational(rhs._den, rhs._num);
}

istream &operator>>(istream &is, Rational &r) {
	int64_t x;
	if (!(is >> x)) {
		return is;
	}
	char c = is.peek();
	if (c == '/') {
		is.get();
		int64_t y;
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
			int64_t z;
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
		char c;
		int e = 0;
		while (true) {
			c = is.peek();
			if (c >= '0' && c <= '9') {
				is.get();
				x = x * 10 + (c - '0');
				--e;
			} else if (c == 'e' || c == 'E') {
				is.get();
				int64_t exp;
				if (!(is >> exp)) {
					r = Rational(x, d);
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
		int d = 1;
		while (e > 0) { x *= 10; --e; }
		while (e < 0) { d *= 10; ++e; }
		r = Rational(x, d);
		return is;
	}
}

ostream &operator<<(ostream &os, Rational const &r) {
	if (r.is_nan()) return os << "NaN";
	if (r.is_inf()) return os << (r.is_positive_inf() ? "INF" : "-INF");
	if (r.is_zero()) return os << "0";
	if (r._den == 1) return os << r._num;
	switch (os.iword(format_idx)) {
	case static_cast<int>(Rational::Format::MIXED):
		auto i = r._num / r._den;
		auto n = r._num % r._den;
		os << i;
		ShowPos showpos(os, true);
		os << n << "/" << r._den;
	case static_cast<int>(Rational::Format::EXACT):
		return os << r._num << "/" << r._den;
	case static_cast<int>(Rational::Format::DECIMAL):
	default:
		auto i = r._num / r._den;
		auto n = safe_abs(r._num % r._den);
		auto precision = os.precision();
		vector<char> buf;
		buf.resize(precision + 1);
		while (n != 0 && precision-- > 0) {
			n *= 10;
			buf.push_back('0' + n / r._den);
			n %= r._den;
		}
		n *= 2;
		if (n > r._den || n == r._den && buf.back() & 1) {
			auto it = buf.rend();
			while (it != buf.rend() && *it == '9') {
				*it++ = '0';
			}
			if (it != buf.rend()) {
				i = i + sign(i);
			}
		}
		os << i << '.' << buf.data();
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
