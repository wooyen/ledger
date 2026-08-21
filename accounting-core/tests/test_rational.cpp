#include <gtest/gtest.h>
#include "ledger/core/rational.hpp"
#include <sstream>

using namespace ledger;

// helper: format Rational as exact fraction string
static std::string exact_str(Rational const& r) {
    std::ostringstream oss;
    oss << exact << r;
    return oss.str();
}

// helper: parse Rational from string
static Rational parse(std::string_view sv) {
    std::istringstream iss(std::string{sv});
    Rational r;
    iss >> r;
    return r;
}

// ── Construction ──────────────────────────────────────────────────

TEST(RationalTest, DefaultConstruction) {
    Rational r;
    EXPECT_TRUE(r.is_zero());
    EXPECT_FALSE(r.is_nan());
    EXPECT_FALSE(r.is_inf());
}

TEST(RationalTest, IntegerConstruction) {
    Rational r(42);
    EXPECT_FALSE(r.is_zero());
    EXPECT_EQ(exact_str(r), "42");
}

TEST(RationalTest, FractionAutoReduce) {
    Rational r(6, 4);
    EXPECT_EQ(exact_str(r), "3/2");
}

TEST(RationalTest, NegativeNumerator) {
    Rational r(-3, 4);
    EXPECT_EQ(exact_str(r), "-3/4");
}

TEST(RationalTest, NegativeDenominator) {
    Rational r(1, static_cast<int64_t>(-2));
    EXPECT_EQ(exact_str(r), "-1/2");
}

TEST(RationalTest, ZeroDenominatorCreatesInf) {
    Rational r(1, 0U);
    EXPECT_TRUE(r.is_inf());
    EXPECT_TRUE(r.is_positive_inf());
}

// ── Special values ────────────────────────────────────────────────

TEST(RationalTest, NaN) {
    EXPECT_TRUE(Rational::NaN.is_nan());
    EXPECT_FALSE(Rational::NaN.is_zero());
    EXPECT_FALSE(Rational::NaN.is_inf());
}

TEST(RationalTest, PositiveInfinity) {
    EXPECT_TRUE(Rational::POSINF.is_inf());
    EXPECT_TRUE(Rational::POSINF.is_positive_inf());
    EXPECT_FALSE(Rational::POSINF.is_negative_inf());
}

TEST(RationalTest, NegativeInfinity) {
    EXPECT_TRUE(Rational::NEGINF.is_inf());
    EXPECT_TRUE(Rational::NEGINF.is_negative_inf());
    EXPECT_FALSE(Rational::NEGINF.is_positive_inf());
}

// ── Arithmetic ────────────────────────────────────────────────────

TEST(RationalTest, Addition) {
    Rational a(3, 2);   // 3/2
    Rational b(5, 4);   // 5/4
    EXPECT_EQ(a + b, Rational(11, 4));
}

TEST(RationalTest, AdditionWithNaN) {
    auto r = Rational(1, 1) + Rational::NaN;
    EXPECT_TRUE(r.is_nan());
}

TEST(RationalTest, Subtraction) {
    Rational a(5, 2);   // 5/2
    Rational b(1, 2);   // 1/2
    EXPECT_EQ(a - b, Rational(2));
}

TEST(RationalTest, Multiplication) {
    Rational a(3, 2);   // 3/2
    Rational b(4, 3);   // 4/3
    EXPECT_EQ(a * b, Rational(2));
}

TEST(RationalTest, Division) {
    Rational a(3, 2);   // 3/2
    Rational b(3, 1);   // 3
    EXPECT_EQ(a / b, Rational(1, 2));
}

TEST(RationalTest, DivisionByZeroYieldsInf) {
    Rational a(1, 1);
    Rational b(0, 1);
    EXPECT_TRUE((a / b).is_inf());
}

TEST(RationalTest, Negation) {
    Rational a(3, 4);
    EXPECT_EQ(-a, Rational(-3, 4));
}

TEST(RationalTest, NegationAddsToZero) {
    Rational a(5, 1);
    EXPECT_TRUE((a + (-a)).is_zero());
}

// ── Comparison ────────────────────────────────────────────────────

TEST(RationalTest, EqualFractions) {
    Rational a(3, 2);    // 3/2
    Rational b(15, 10);  // 15/10, auto-reduced to 3/2
    EXPECT_EQ(a, b);
}

TEST(RationalTest, LessThan) {
    Rational a(1, 3);
    Rational b(1, 2);
    EXPECT_LT(a, b);
}

TEST(RationalTest, GreaterThan) {
    Rational a(1, 2);
    Rational b(1, 4);
    EXPECT_GT(a, b);
}

TEST(RationalTest, NaNComparisonIsUnordered) {
    auto cmp = Rational::NaN <=> Rational(1, 1);
    EXPECT_EQ(cmp, std::partial_ordering::unordered);
}

// ── Input (operator>>) ────────────────────────────────────────────

TEST(RationalTest, InputStreamFraction) {
    auto r = parse("3/4");
    EXPECT_EQ(r, Rational(3, 4));
}

TEST(RationalTest, InputStreamNegativeFraction) {
    auto r = parse("-5/8");
    EXPECT_EQ(r, Rational(-5, 8));
}

TEST(RationalTest, InputStreamDecimal) {
    auto r = parse("1.5");
    EXPECT_EQ(r, Rational(3, 2));
}

TEST(RationalTest, InputStreamMixedNumber) {
    auto r = parse("2+1/3");
    EXPECT_EQ(r, Rational(7, 3));
}

TEST(RationalTest, InputStreamMixedNegative) {
    auto r = parse("2-1/3");
    EXPECT_EQ(r, Rational(5, 3));
}

// ── Output (operator<<) ───────────────────────────────────────────

TEST(RationalTest, OutputExact) {
    Rational r(-3, 4);
    EXPECT_EQ(exact_str(r), "-3/4");
}

TEST(RationalTest, OutputMixed) {
    Rational r(7, 3);
    std::ostringstream oss;
    oss << mixed << r;
    EXPECT_EQ(oss.str(), "2+1/3");
}

TEST(RationalTest, OutputInteger) {
    Rational r(42, 1);
    std::ostringstream oss;
    oss << r;
    EXPECT_EQ(oss.str(), "42");
}

TEST(RationalTest, OutputNaN) {
    std::ostringstream oss;
    oss << Rational::NaN;
    EXPECT_EQ(oss.str(), "NaN");
}

TEST(RationalTest, OutputINF) {
    std::ostringstream oss;
    oss << Rational::POSINF;
    EXPECT_EQ(oss.str(), "INF");
}

TEST(RationalTest, OutputNegativeINF) {
    std::ostringstream oss;
    oss << Rational::NEGINF;
    EXPECT_EQ(oss.str(), "-INF");
}

TEST(RationalTest, OutputDecimal) {
    Rational r(1, 4);
    std::ostringstream oss;
    oss << decimal << r;
    EXPECT_EQ(oss.str(), "0.25");
}

TEST(RationalTest, OutputDecimalNegative) {
    Rational r(-1, 4);
    std::ostringstream oss;
    oss << decimal << r;
    EXPECT_EQ(oss.str(), "-0.25");
}

TEST(RationalTest, OutputDecimalCycle) {
	Rational r1(1, 3);
	Rational r2(2, 3);
	Rational r3(3, 3);
	std::ostringstream oss;
	oss << decimal << precision(4) << r1;
	EXPECT_EQ(oss.str(), "0.3333");
	oss.str("");
	oss << decimal << precision(4) << r2;
	EXPECT_EQ(oss.str(), "0.6667");
	oss.str("");
	oss << decimal << fixed << precision(4) << r3;
	EXPECT_EQ(oss.str(), "1.0000");
}
