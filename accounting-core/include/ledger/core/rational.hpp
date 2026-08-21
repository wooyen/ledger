#pragma once
#include <cstdint>
#include <compare>
#include <string>
#include <optional>

namespace ledger {

// 有理数：value = num / denom，支持任意精度（只要不溢出 int64）
class Rational {
	int64_t _num   = 0;
	uint64_t _den = 1;
public:
	enum class Format {DECIMAL, MIXED, EXACT};

	static const Rational NaN;
	static const Rational POSINF;
	static const Rational NEGINF;

	Rational() = default;
	Rational(int64_t n, uint64_t d = 1);
	Rational(int64_t n, int64_t d);

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
