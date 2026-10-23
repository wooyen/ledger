#include <gtest/gtest.h>
#include "ledger/core/rational.hpp"
#include "ledger/core/rational_format.hpp"
#include <sstream>
#include <iomanip>
#include <format>
#include <stdexcept>

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
    auto r = parse("2&1/3");
    EXPECT_EQ(r, Rational(7, 3));
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
    EXPECT_EQ(oss.str(), "2&1/3");
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
    EXPECT_EQ(oss.str(), "nan");
}

TEST(RationalTest, OutputINF) {
    std::ostringstream oss;
    oss << Rational::POSINF;
    EXPECT_EQ(oss.str(), "inf");
}

TEST(RationalTest, OutputNegativeINF) {
    std::ostringstream oss;
    oss << Rational::NEGINF;
    EXPECT_EQ(oss.str(), "-inf");
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
	oss << decimal << std::setprecision(4) << r1;
	EXPECT_EQ(oss.str(), "0.3333");
	oss.str("");
	oss << decimal << std::setprecision(4) << r2;
	EXPECT_EQ(oss.str(), "0.6667");
	oss.str("");
	oss << decimal << std::fixed << std::setprecision(4) << r3;
	EXPECT_EQ(oss.str(), "1.0000");
}

// ══════════════════════════════════════════════════════════════════
//  Stream I/O (operator>> / operator<<) & Format I/O (std::formatter)
//  以下测试覆盖「流式输入输出」与「格式化输入输出」两大部分。
// ══════════════════════════════════════════════════════════════════

// helper: 解析字符串；若流处于失败状态返回 false
static bool try_parse(std::string_view sv, Rational& out) {
	std::istringstream iss(std::string{sv});
	Rational r;
	iss >> r;
	if (iss.fail()) return false;
	out = r;
	return true;
}

// helper: 以指定进制（8/10/16）解析
static Rational parse_base(std::string_view sv, int base) {
	std::istringstream iss(std::string{sv});
	if (base == 16) iss >> std::hex;
	else if (base == 8) iss >> std::oct;
	Rational r;
	iss >> r;
	return r;
}

// helper: 运行期格式化（vformat），便于测试非法格式说明符
static std::string fmt(std::string_view spec, Rational const& r) {
	return std::vformat(spec, std::make_format_args(r));
}

// ── 流式输入：整数 / 分数 / 带分数 / 小数 ─────────────────────────

TEST(RationalStreamInput, Integer) {
	Rational r;
	ASSERT_TRUE(try_parse("42", r));
	EXPECT_EQ(r, Rational(42));
}

TEST(RationalStreamInput, PositiveSignedInteger) {
	Rational r;
	ASSERT_TRUE(try_parse("+7", r));
	EXPECT_EQ(r, Rational(7));
}

TEST(RationalStreamInput, NegativeZero) {
	Rational r;
	ASSERT_TRUE(try_parse("-0", r));
	EXPECT_TRUE(r.is_zero());
}

TEST(RationalStreamInput, LeadingDotDecimal) {
	Rational r;
	ASSERT_TRUE(try_parse(".25", r));
	EXPECT_EQ(r, Rational(1, 4));
}

TEST(RationalStreamInput, NegativeLeadingDotDecimal) {
	Rational r;
	ASSERT_TRUE(try_parse("-.25", r));
	EXPECT_EQ(r, Rational(-1, 4));
}

TEST(RationalStreamInput, TrailingDotInteger) {
	Rational r;
	ASSERT_TRUE(try_parse("3.", r));
	EXPECT_EQ(r, Rational(3));
}

TEST(RationalStreamInput, DecimalTrailingZerosReduce) {
	Rational r;
	ASSERT_TRUE(try_parse("2.50", r));
	EXPECT_EQ(r, Rational(5, 2));
}

TEST(RationalStreamInput, ScientificNotation) {
	Rational r;
	ASSERT_TRUE(try_parse("1.5e3", r));
	EXPECT_EQ(r, Rational(1500));
}

TEST(RationalStreamInput, ZeroNumeratorFraction) {
	Rational r;
	ASSERT_TRUE(try_parse("0/5", r));
	EXPECT_TRUE(r.is_zero());
}

TEST(RationalStreamInput, FractionZeroDenominatorIsInf) {
	Rational r;
	ASSERT_TRUE(try_parse("1/0", r));
	EXPECT_TRUE(r.is_positive_inf());
}

TEST(RationalStreamInput, NegativeMixedNumber) {
	Rational r;
	ASSERT_TRUE(try_parse("-2&1/3", r));
	EXPECT_EQ(r, Rational(-7, 3));
}

TEST(RationalStreamInput, StopsAfterValidPrefix) {
	// “1/2/3” 只应消费 “1/2”，且不应置失败位
	Rational r;
	ASSERT_TRUE(try_parse("1/2/3", r));
	EXPECT_EQ(r, Rational(1, 2));
	ASSERT_TRUE(try_parse("1.5.5", r));
	EXPECT_EQ(r, Rational(3, 2));
}

// ── 流式输入：特殊符号 ────────────────────────────────────────────

TEST(RationalStreamInput, NanSymbol) {
	Rational r;
	ASSERT_TRUE(try_parse("nan", r));
	EXPECT_TRUE(r.is_nan());
	ASSERT_TRUE(try_parse("NAN", r));
	EXPECT_TRUE(r.is_nan());
}

TEST(RationalStreamInput, InfSymbols) {
	Rational r;
	ASSERT_TRUE(try_parse("inf", r));
	EXPECT_TRUE(r.is_positive_inf());
	ASSERT_TRUE(try_parse("INF", r));
	EXPECT_TRUE(r.is_positive_inf());
	ASSERT_TRUE(try_parse("+inf", r));
	EXPECT_TRUE(r.is_positive_inf());
	ASSERT_TRUE(try_parse("-inf", r));
	EXPECT_TRUE(r.is_negative_inf());
}

// ── 流式输入：进制（basefield）────────────────────────────────────

TEST(RationalStreamInput, HexBase) {
	EXPECT_EQ(parse_base("1f", 16), Rational(31));
	EXPECT_EQ(parse_base("ff", 16), Rational(255));
}

TEST(RationalStreamInput, OctalBase) {
	EXPECT_EQ(parse_base("17", 8), Rational(15));
}

TEST(RationalStreamInput, HexWithFraction) {
	// 0x1f / 0x2 == 31/2
	EXPECT_EQ(parse_base("1f/2", 16), Rational(31, 2));
}

// ── 流式输入：非法输入应置 failbit ────────────────────────────────

TEST(RationalStreamInput, InvalidInputsSetFailBit) {
	Rational dummy;
	for (auto* bad : {"", "abc", "-", "3/", "3&", "3&/4", "2&1/", "/4"}) {
		EXPECT_FALSE(try_parse(bad, dummy));
	}
}

// ── 流式输出：exact / mixed 模式 ──────────────────────────────────

TEST(RationalStreamOutput, ExactFraction) {
	EXPECT_EQ(exact_str(Rational(7, 3)), "7/3");
	EXPECT_EQ(exact_str(Rational(-7, 3)), "-7/3");
	EXPECT_EQ(exact_str(Rational(1, 3)), "1/3");
}

TEST(RationalStreamOutput, MixedFraction) {
	std::ostringstream oss;
	oss << mixed << Rational(7, 3);
	EXPECT_EQ(oss.str(), "2&1/3");
	oss.str("");
	oss << mixed << Rational(-7, 3);
	EXPECT_EQ(oss.str(), "-2&1/3");
	// 整数部分为 0 时退化为真分数，不打印 “0&”
	oss.str("");
	oss << mixed << Rational(1, 3);
	EXPECT_EQ(oss.str(), "1/3");
}

TEST(RationalStreamOutput, DefaultsToDecimal) {
	// 未设置任何操纵符时，默认使用 DECIMAL 表示
	std::ostringstream oss;
	oss << Rational(3, 4);
	EXPECT_EQ(oss.str(), "0.75");
}

TEST(RationalStreamOutput, SpecialValuesDefault) {
	std::ostringstream oss;
	oss << Rational::NaN;
	EXPECT_EQ(oss.str(), "nan");
	oss.str("");
	oss << Rational::POSINF;
	EXPECT_EQ(oss.str(), "inf");
	oss.str("");
	oss << Rational::NEGINF;
	EXPECT_EQ(oss.str(), "-inf");
}

TEST(RationalStreamOutput, IntegerWithBaseFlags) {
	Rational i(255);
	// 每个子用例使用独立的流，避免格式标志（showbase/uppercase）在断言间残留
	std::ostringstream h;
	h << std::hex << exact << i;
	EXPECT_EQ(h.str(), "ff");
	std::ostringstream hb;
	hb << std::hex << std::showbase << exact << i;
	EXPECT_EQ(hb.str(), "0xff");
	std::ostringstream hu;
	hu << std::hex << std::uppercase << std::showbase << exact << i;
	EXPECT_EQ(hu.str(), "0XFF");
	std::ostringstream o;
	o << std::oct << exact << i;
	EXPECT_EQ(o.str(), "377");
}

TEST(RationalStreamOutput, ShowPosOnFraction) {
	std::ostringstream oss;
	oss << std::showpos << exact << Rational(7, 3);
	EXPECT_EQ(oss.str(), "+7/3");
}

TEST(RationalStreamOutput, HexBaseOnFractionAndMixed) {
	std::ostringstream oss;
	oss << std::hex << std::showbase << exact << Rational(7, 3);
	EXPECT_EQ(oss.str(), "0x7/0x3");
	oss.str("");
	oss << std::hex << std::showbase << mixed << Rational(7, 3);
	EXPECT_EQ(oss.str(), "0x2&0x1/0x3");
}

TEST(RationalStreamOutput, DecimalFixedPrecision) {
	std::ostringstream oss;
	oss << decimal << std::fixed << std::setprecision(3) << Rational(1, 3);
	EXPECT_EQ(oss.str(), "0.333");
}

// ── 输入/输出往返一致性 ───────────────────────────────────────────

TEST(RationalStreamIO, RoundTrip) {
	for (Rational v : {Rational(7, 3), Rational(-7, 3), Rational(1, 2), Rational(42)}) {
		std::ostringstream out;
		out << exact << v;
		Rational back;
		ASSERT_TRUE(try_parse(out.str(), back));
		EXPECT_EQ(back, v);
	}
}

// ── 回归：本轮针对 operator>> 修复的三处缺陷 ────────────────────────

// 缺陷①：纯整数恰好在 EOF 结束曾被随后的 is.peek() 的 sentry 追加置
//         failbit（值是 42 但 fail()==true）；修复：在 peek 前加 is.eof() 短路。
//         本用例锁死：值正确 + !fail() + eofbit 允许置位（eof != fail）。
TEST(RationalRegression, PlainIntegerAtEofDoesNotSetFailBit) {
	for (auto* s : {"42", "7", "0", "-0", "-5", "1234567"}) {
		std::istringstream iss(s);
		Rational r;
		iss >> r;
		EXPECT_FALSE(iss.fail());
		EXPECT_TRUE(iss.eof());
	}
	{
		std::istringstream iss("42"); Rational r; iss >> r;
		EXPECT_EQ(r, Rational(42));
	}
	{
		std::istringstream iss("-5"); Rational r; iss >> r;
		EXPECT_EQ(r, Rational(-5));
	}
	{
		std::istringstream iss("-0"); Rational r; iss >> r;
		EXPECT_TRUE(r.is_zero());
	}
}

// 缺陷②：不带符号的 nan/inf/NAN/INF 曾被 L199 的
//         「first != '-' && first != '+'」守卫提前 return 拒收；修复：删除该守卫。
//         本用例覆盖无符号（含大小写混排）+ 带符号的 nan/inf。
TEST(RationalRegression, UnsignedSpecialLiteralsParse) {
	struct Case { const char* src; bool expect_nan; bool expect_posinf; };
	for (Case c : {Case{"nan", true, false}, Case{"NAN", true, false},
	                Case{"NaN", true, false}, Case{"nAN", true, false},
	                Case{"inf", false, true}, Case{"INF", false, true},
	                Case{"Inf", false, true}, Case{"iNF", false, true}}) {
		std::istringstream iss(c.src);
		Rational r;
		iss >> r;
		EXPECT_FALSE(iss.fail());
		if (c.expect_nan)   EXPECT_TRUE(r.is_nan());
		if (c.expect_posinf) EXPECT_TRUE(r.is_positive_inf());
	}
	// 带符号形式仍正确（回归基线，不因守卫删除而回退）
	{
		std::istringstream iss("-inf"); Rational r; iss >> r;
		EXPECT_FALSE(iss.fail());
		EXPECT_TRUE(r.is_negative_inf());
	}
	{
		std::istringstream iss("+inf"); Rational r; iss >> r;
		EXPECT_FALSE(iss.fail());
		EXPECT_TRUE(r.is_positive_inf());
	}
}

// 缺陷③：前导小数点分支此前漏消费 '.'，导致 parse_decimal 里 while 不进入。
//         修复方案把 is.get() 挪入 parse_decimal 首行；主小数路径同样如此。
//         本用例锁死：前导 '.'、尾随 '.'、带符号前导 '.' 三类都以正确值/无 failbit。
TEST(RationalRegression, LeadingAndTrailingDotDecimals) {
	struct T { const char* src; Rational expect; };
	for (T t : {T{".25", Rational(1, 4)},
	             T{"-.25", Rational(-1, 4)},
	             T{"+.7", Rational(7, 10)},
	             T{"3.", Rational(3)},
	             T{".5", Rational(1, 2)},
	             T{"-.5", Rational(-1, 2)},
	             T{".125", Rational(1, 8)}}) {
		Rational r;
		ASSERT_TRUE(try_parse(t.src, r));
		EXPECT_EQ(r, t.expect);
	}
}

// 缺陷①补充：多 token 中最后一个恰在 EOF 结束，前面与最后都不应误置 failbit。
TEST(RationalRegression, LastTokenAtEofAmongSequence) {
	std::istringstream iss("3/4 42 1.5");
	Rational a, b, c;
	iss >> a; EXPECT_FALSE(iss.fail()); EXPECT_EQ(a, Rational(3, 4));
	iss >> b; EXPECT_FALSE(iss.fail()); EXPECT_EQ(b, Rational(42));
	iss >> c; EXPECT_FALSE(iss.fail()); EXPECT_EQ(c, Rational(3, 2));
	EXPECT_TRUE(iss.eof());
}

// 缺陷①语义核心：eofbit 与 failbit 是不同的状态位，读到流尾是正常的。
TEST(RationalRegression, EofBitIsNotFailBit) {
	std::istringstream iss("42");
	Rational r;
	iss >> r;
	EXPECT_FALSE(iss.fail());
	EXPECT_FALSE(iss.bad());
	EXPECT_TRUE(iss.eof());
}

// ══════════════════════════════════════════════════════════════════
//  Format I/O —— std::formatter<ledger::Rational>
// ══════════════════════════════════════════════════════════════════

TEST(RationalFormat, DecimalMode) {
	// {:d} 将剩余说明符委托给 formatter<long double>
	EXPECT_EQ(std::format("{:d}", Rational(1, 4)), "0.25");
	EXPECT_EQ(fmt("{:d.4}", Rational(1, 3)), "0.3333");
	EXPECT_EQ(fmt("{:d9.4}", Rational(1, 3)), "   0.3333");
	EXPECT_EQ(fmt("{:df}", Rational(1, 3)), "0.333333");
	EXPECT_EQ(fmt("{:de}", Rational(22, 7)), "3.142857e+00");
	EXPECT_EQ(fmt("{:d<8.2}", Rational(1, 3)), "0.33    ");
}

TEST(RationalFormat, MixedAndFractionMode) {
	Rational a(7, 3);
	EXPECT_EQ(std::format("{:m}", a), "2&1/3");
	EXPECT_EQ(std::format("{:f}", a), "7/3");
	// 默认模式（无说明符）为 mixed
	EXPECT_EQ(std::format("{}", a), "2&1/3");
	EXPECT_EQ(std::format("{:m}", Rational(-7, 3)), "-2&1/3");
	EXPECT_EQ(std::format("{:f}", Rational(-7, 3)), "-7/3");
}

TEST(RationalFormat, SignSpecifier) {
	Rational a(7, 3);
	EXPECT_EQ(std::format("{:m+}", a), "+2&1/3");
	EXPECT_EQ(std::format("{:f+}", a), "+7/3");
	EXPECT_EQ(std::format("{:f+}", Rational(-7, 3)), "-7/3");
	// 空格符号：正数前加空格
	EXPECT_EQ(std::format("{:m }", a), " 2&1/3");
}

TEST(RationalFormat, WidthFillAndAlignment) {
	Rational a(7, 3);
	// [fill][align]，默认填充为空格、右对齐
	EXPECT_EQ(std::format("{:m>10}", a), "     2&1/3");
	EXPECT_EQ(std::format("{:f<10}", a), "7/3       ");
	EXPECT_EQ(std::format("{:f^10}", a), "   7/3    ");
	// 自定义填充字符 '0'
	EXPECT_EQ(std::format("{:f0>8}", a), "000007/3");
}

TEST(RationalFormat, DynamicWidth) {
	Rational a(7, 3);
	EXPECT_EQ(std::format("{:m>{}}", a, 10), "     2&1/3");
	EXPECT_EQ(std::format("{:m>{}}", a, 8), "   2&1/3");
}

TEST(RationalFormat, IntegerBaseTypes) {
	Rational i(42);
	EXPECT_EQ(std::format("{:fx}", i), "2a");
	EXPECT_EQ(std::format("{:f#x}", i), "0x2a");
	EXPECT_EQ(std::format("{:fo}", i), "52");
	EXPECT_EQ(std::format("{:fb}", i), "101010");
	EXPECT_EQ(std::format("{:f#b}", i), "0b101010");
	Rational big(1000);
	EXPECT_EQ(std::format("{:fX}", big), "3E8");
	EXPECT_EQ(std::format("{:fO}", big), "1750");
	EXPECT_EQ(std::format("{:f#B}", big), "0B1111101000");
}

TEST(RationalFormat, IntegerSignAndWidth) {
	Rational i(42);
	EXPECT_EQ(std::format("{:m>6}", i), "    42");
	EXPECT_EQ(std::format("{:m+}", i), "+42");
}

TEST(RationalFormat, FractionBasePrefix) {
	// 非整数（分母!=1）也支持进制与 showbase，num/den 各自进位
	EXPECT_EQ(std::format("{:f#x}", Rational(7, 3)), "0x7/0x3");
	// 八进制 showbase 为每个部分加上前缀 "0"
	EXPECT_EQ(std::format("{:m#o}", Rational(7, 3)), "02&01/03");
}

TEST(RationalFormat, SpecialValues) {
	EXPECT_EQ(std::format("{:m}", Rational::NaN), "nan");
	EXPECT_EQ(std::format("{:f}", Rational::NaN), "nan");
	EXPECT_EQ(std::format("{:d}", Rational::NaN), "nan");
	EXPECT_EQ(std::format("{:m}", Rational::POSINF), "inf");
	EXPECT_EQ(std::format("{:m}", Rational::NEGINF), "-inf");
	EXPECT_EQ(std::format("{:d}", Rational::POSINF), "inf");
}

TEST(RationalFormat, InvalidSpecifierThrows) {
	Rational a(7, 3);
	// 非法的模式字符
	EXPECT_THROW(fmt("{:q}", a), std::format_error);
	// 动态宽度参数不是整数
	std::string not_an_int = "x";
	EXPECT_THROW(std::vformat("{:m>{}}", std::make_format_args(a, not_an_int)),
	             std::format_error);
}
