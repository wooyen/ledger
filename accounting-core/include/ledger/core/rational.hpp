#pragma once
#include <cstdint>
#include <compare>
#include <functional>
#include <iosfwd>
#include "ledger/core/util.hpp"

namespace ledger {

// 有理数：value = num / denom，支持任意精度（只要不溢出 int64）
class Rational {
	int64_t _num = 0;
	uint64_t _den = 1;
	void reduce();
public:
	enum class Format {DECIMAL, MIXED, EXACT};

	static const Rational ZERO;
	static const Rational ONE;
	static const Rational NaN;
	static const Rational POSINF;
	static const Rational NEGINF;

	Rational() = default;
	Rational(int64_t n) : _num(n) {};
	template<typename T1, typename T2>
	requires std::is_integral_v<T1> && std::is_integral_v<T2> && std::is_unsigned_v<T2>
	Rational(T1 n, T2 d) : _num(n), _den(d) {
		reduce();
	};
	template<typename T1, typename T2>
	requires std::is_integral_v<T1> && std::is_integral_v<T2> && std::is_signed_v<T2>
	Rational(T1 n, T2 d) : _num(n), _den(__internal::abs(d)) {
		_num = d < 0 ? -_num : _num;
		reduce();
	};

	std::partial_ordering operator<=>(Rational const&) const;
	bool operator==(Rational const&) const = default;

	Rational operator-() const { return Rational{-_num, _den}; }

	Rational& operator+=(Rational const&);
	Rational& operator-=(Rational const&);
	Rational& operator*=(Rational const&);
	Rational& operator/=(Rational const&);

	Rational operator+(Rational const& rhs) const {
		Rational tmp = *this;
		tmp += rhs;
		return tmp;
	}
	Rational operator-(Rational const& rhs) const {
		Rational tmp = *this;
		tmp -= rhs;
		return tmp;
	}
	Rational operator*(Rational const& rhs) const {
		Rational tmp = *this;
		tmp *= rhs;
		return tmp;
	}
	Rational operator/(Rational const& rhs) const {
		Rational tmp = *this;
		tmp /= rhs;
		return tmp;
	}

	int64_t numerator() const { return _num; }
	uint64_t denominator() const { return _den; }

	bool is_zero() const { return _num == 0 && _den != 0; }
	bool is_nan() const { return _den == 0 && _num == 0; }
	bool is_inf() const { return _den == 0 && _num != 0; }
	bool is_positive_inf() const { return _den == 0 && _num > 0; }
	bool is_negative_inf() const { return _den == 0 && _num < 0; }

	friend std::istream& operator>>(std::istream&, Rational&);
	friend std::ostream& operator<<(std::ostream&, Rational const&);

};

std::ostream &decimal(std::ostream &os);
std::ostream &mixed(std::ostream &os);
std::ostream &exact(std::ostream &is);
} // namespace ledger
namespace std {
	template <> struct hash<ledger::Rational> {
		size_t operator()(const ledger::Rational& r) const {
			return hash<int64_t>()(r.numerator()) ^ hash<uint64_t>()(r.denominator());
		}
	};
}
