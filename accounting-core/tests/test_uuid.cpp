#include <gtest/gtest.h>
#include "ledger/core/uuid.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

using namespace ledger;

namespace {

// Fixed byte pattern 0x01..0x10, used to pin down byte order in both
// directions (parse and format).
constexpr std::string_view kCompactLower = "0102030405060708090a0b0c0d0e0f10";
constexpr std::string_view kCompactUpper = "0102030405060708090A0B0C0D0E0F10";
constexpr std::string_view kDashedUpper  = "01020304-0506-0708-090A-0B0C0D0E0F10";
constexpr std::string_view kDashedLower  = "01020304-0506-0708-090a-0b0c0d0e0f10";
constexpr std::string_view kNullDashed   = "00000000-0000-0000-0000-000000000000";

Uuid sequential() { return Uuid::from_string(kCompactLower); }

// Decode the 48-bit big-endian millisecond timestamp prefix of a UUIDv7.
uint64_t timestamp_ms(Uuid const& u) {
	uint64_t ts = 0;
	for (int i = 0; i < 6; ++i) ts = (ts << 8) | u.bytes[i];
	return ts;
}

int64_t now_ms() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Runtime formatting: invalid specifiers must surface as exceptions instead
// of compile-time errors, so std::vformat is used deliberately.
std::string vfmt(std::string_view spec, Uuid const& u) {
	return std::vformat(spec, std::make_format_args(u));
}

} // namespace

// ── Value semantics ───────────────────────────────────────────────

TEST(UuidTest, DefaultConstructedIsNull) {
	Uuid u;
	EXPECT_TRUE(u.is_null());
	EXPECT_EQ(u, Uuid{});
}

TEST(UuidTest, DefaultConstructedBytesAreZero) {
	Uuid u;
	for (int i = 0; i < 16; ++i) {
		EXPECT_EQ(u.bytes[i], 0) << "byte index " << i;
	}
}

TEST(UuidTest, EqualityComparesBytes) {
	EXPECT_EQ(sequential(), sequential());

	Uuid other = sequential();
	other.bytes[15] = 0xFF;
	EXPECT_NE(sequential(), other);
}

TEST(UuidTest, OrderingIsLexicographicByByte) {
	Uuid low{};
	Uuid high{};
	high.bytes[0]  = 0x01;
	low.bytes[15]  = 0xFF;   // lower significance byte must not outweigh bytes[0]
	EXPECT_LT(low, high);
	EXPECT_GT(high, low);
}

TEST(UuidTest, ComparableInOrderedContainer) {
	Uuid zero{};
	Uuid one{};
	one.bytes[0] = 1;
	Uuid two{};
	two.bytes[0] = 2;

	std::set<Uuid> set{two, zero, one};
	ASSERT_EQ(set.size(), 3u);
	auto it = set.begin();
	EXPECT_EQ(*it++, zero);
	EXPECT_EQ(*it++, one);
	EXPECT_EQ(*it, two);
}

TEST(UuidTest, IsNullDetectsAnyNonZeroByte) {
	for (int idx : {0, 7, 15}) {
		Uuid u;
		u.bytes[idx] = 1;
		EXPECT_FALSE(u.is_null()) << "byte index " << idx;
	}

	Uuid u;
	u.bytes[15] = 0xFF;
	EXPECT_FALSE(u.is_null());
}

TEST(UuidTest, IsNullAcceptsAllZeroBytes) {
	Uuid u;   // default member initialiser zeroes the array
	EXPECT_TRUE(u.is_null());
	for (int i = 1; i < 16; ++i) u.bytes[i] = 0x00;
	EXPECT_TRUE(u.is_null());
}

TEST(UuidTest, HashIsStableAndConsistent) {
	Uuid u = sequential();
	EXPECT_EQ(std::hash<Uuid>{}(u), std::hash<Uuid>{}(sequential()));
	EXPECT_EQ(std::hash<Uuid>{}(u), std::hash<Uuid>{}(u));
}

TEST(UuidTest, HashUsableInUnorderedSet) {
	std::unordered_set<Uuid> set;
	set.insert(sequential());
	set.insert(sequential());   // duplicate must collapse to one entry

	Uuid other{};
	other.bytes[0] = 0xFF;
	set.insert(other);

	EXPECT_EQ(set.size(), 2u);
	EXPECT_EQ(set.count(sequential()), 1u);
}

// ── Parsing (from_string) ─────────────────────────────────────────

TEST(UuidParseTest, ParsesCompactLowercase) {
	EXPECT_EQ(Uuid::from_string(kCompactLower), sequential());
}

TEST(UuidParseTest, ParsesCompactUppercase) {
	EXPECT_EQ(Uuid::from_string(kCompactUpper), sequential());
}

TEST(UuidParseTest, ParsesCanonicalDashedForms) {
	EXPECT_EQ(Uuid::from_string(kDashedUpper), sequential());
	EXPECT_EQ(Uuid::from_string(kDashedLower), sequential());
}

TEST(UuidParseTest, ParsesBracedForm) {
	EXPECT_EQ(Uuid::from_string("{01020304-0506-0708-090A-0B0C0D0E0F10}"), sequential());
}

TEST(UuidParseTest, IgnoresSurroundingWhitespace) {
	EXPECT_EQ(Uuid::from_string("  0102030405060708090a0b0c0d0e0f10  "), sequential());
}

TEST(UuidParseTest, IgnoresArbitrarySeparators) {
	// Every non hex character is skipped, not just '-'.
	EXPECT_EQ(Uuid::from_string("01_02:03|04 05/06.07+08=09(0a)0b[0c]0d{0e}0f#10"),
	          sequential());
}

TEST(UuidParseTest, IsCaseInsensitive) {
	EXPECT_EQ(Uuid::from_string("aabbccddeeff00112233445566778899"),
	          Uuid::from_string("AABBCCDDEEFF00112233445566778899"));
}

TEST(UuidParseTest, ByteOrderIsBigEndian) {
	// The first two hex digits fill bytes[0]; a little-endian implementation
	// would yield 0x10..0x01 here.
	Uuid u = sequential();
	for (int i = 0; i < 16; ++i) {
		EXPECT_EQ(u.bytes[i], static_cast<uint8_t>(i + 1)) << "byte index " << i;
	}
}

TEST(UuidParseTest, StopsAfter32HexCharacters) {
	// Trailing hex digits are ignored rather than treated as an error.
	EXPECT_EQ(Uuid::from_string("0102030405060708090a0b0c0d0e0f10deadbeef"), sequential());
	EXPECT_EQ(Uuid::from_string("0102030405060708090a0b0c0d0e0f10" "01"), sequential());
}

TEST(UuidParseTest, ParsesNullUuid) {
	EXPECT_TRUE(Uuid::from_string(kNullDashed).is_null());
	EXPECT_TRUE(Uuid::from_string("00000000000000000000000000000000").is_null());
	EXPECT_EQ(Uuid::from_string(kNullDashed).to_string(), std::string(kNullDashed));
}

TEST(UuidParseTest, ThrowsWhenFewerThan32HexCharacters) {
	// 31 hex digits: one short of a complete UUID.
	EXPECT_THROW(Uuid::from_string("0102030405060708090a0b0c0d0e0f1"), std::invalid_argument);
	EXPECT_THROW(Uuid::from_string(""), std::invalid_argument);
	EXPECT_THROW(Uuid::from_string("-"), std::invalid_argument);
	EXPECT_THROW(Uuid::from_string("not-a-uuid"), std::invalid_argument);
}

TEST(UuidParseTest, ThrowsInvalidArgumentWithDiagnostic) {
	try {
		Uuid::from_string("xyz");
		FAIL() << "expected std::invalid_argument";
	} catch (std::invalid_argument const& e) {
		EXPECT_STREQ(e.what(), "Invalid UUID string");
	}
}

// ── Serialisation (to_string) ─────────────────────────────────────

TEST(UuidFormatTest, ToStringDefaultsToUppercase) {
	EXPECT_EQ(sequential().to_string(), std::string(kDashedUpper));
}

TEST(UuidFormatTest, ToStringLowercase) {
	EXPECT_EQ(sequential().to_string(false), std::string(kDashedLower));
}

TEST(UuidFormatTest, ToStringIs36CharsWithFourDashes) {
	std::string s = sequential().to_string();
	ASSERT_EQ(s.size(), 36u);

	int dashes = 0;
	for (size_t i = 0; i < s.size(); ++i) {
		bool expect_dash = (i == 8 || i == 13 || i == 18 || i == 23);
		EXPECT_EQ(s[i] == '-', expect_dash) << "position " << i;
		if (expect_dash) ++dashes;
	}
	EXPECT_EQ(dashes, 4);
}

TEST(UuidFormatTest, ToStringOfNull) {
	EXPECT_EQ(Uuid{}.to_string(), std::string(kNullDashed));
	EXPECT_EQ(Uuid{}.to_string(false), std::string(kNullDashed));
}

TEST(UuidFormatTest, ToStringIsBigEndian) {
	// Highest byte of bytes[0] comes first in the string.
	Uuid u{};
	u.bytes[0] = 0xAB;
	EXPECT_EQ(u.to_string().substr(0, 2), "AB");
	EXPECT_EQ(u.to_string(false).substr(0, 2), "ab");
}

TEST(UuidFormatTest, StringRoundTrip) {
	for (Uuid const& u : {sequential(), Uuid{}, Uuid::generate()}) {
		EXPECT_EQ(Uuid::from_string(u.to_string()), u);
		EXPECT_EQ(Uuid::from_string(u.to_string(false)), u);
	}
}

// ── std::formatter ────────────────────────────────────────────────

TEST(UuidFormatterTest, DefaultIsCompactUppercase) {
	EXPECT_EQ(std::format("{}", sequential()), std::string(kCompactUpper));
}

TEST(UuidFormatterTest, EmptySpecifierMatchesDefault) {
	EXPECT_EQ(std::format("{:}", sequential()), std::string(kCompactUpper));
}

TEST(UuidFormatterTest, ExplicitCompactUppercase) {
	EXPECT_EQ(std::format("{:L}", sequential()), std::string(kCompactUpper));
}

TEST(UuidFormatterTest, CompactLowercase) {
	EXPECT_EQ(std::format("{:l}", sequential()), std::string(kCompactLower));
}

TEST(UuidFormatterTest, UppercaseWithDashes) {
	EXPECT_EQ(std::format("{:S}", sequential()), std::string(kDashedUpper));
	EXPECT_EQ(std::format("{:S}", sequential()), sequential().to_string(true));
}

TEST(UuidFormatterTest, LowercaseWithDashes) {
	EXPECT_EQ(std::format("{:s}", sequential()), std::string(kDashedLower));
	EXPECT_EQ(std::format("{:s}", sequential()), sequential().to_string(false));
}

TEST(UuidFormatterTest, FormatsNullUuid) {
	EXPECT_EQ(std::format("{}", Uuid{}), "00000000000000000000000000000000");
	EXPECT_EQ(std::format("{:S}", Uuid{}), std::string(kNullDashed));
}

TEST(UuidFormatterTest, RejectsUnknownTypeSpecifier) {
	EXPECT_THROW(vfmt("{:x}", sequential()), std::format_error);
	EXPECT_THROW(vfmt("{:X}", sequential()), std::format_error);
	EXPECT_THROW(vfmt("{:d}", sequential()), std::format_error);
	EXPECT_THROW(vfmt("{:G}", sequential()), std::format_error);
}

TEST(UuidFormatterTest, RejectsTrailingCharacters) {
	EXPECT_THROW(vfmt("{:LL}", sequential()), std::format_error);
	EXPECT_THROW(vfmt("{:Sx}", sequential()), std::format_error);
	EXPECT_THROW(vfmt("{:S }", sequential()), std::format_error);
}

TEST(UuidFormatterTest, RejectsWidthAndFill) {
	// Width/fill are not supported: '>' is not a valid type specifier.
	EXPECT_THROW(vfmt("{:>40}", sequential()), std::format_error);
	EXPECT_THROW(vfmt("{:*>40}", sequential()), std::format_error);
}

TEST(UuidFormatterTest, WorksInCompoundFormatString) {
	Uuid u = sequential();
	EXPECT_EQ(std::format("<{}|{:s}>", u, u),
	          "<" + std::string(kCompactUpper) + "|" + std::string(kDashedLower) + ">");
}

// ── UUIDv7 generation ─────────────────────────────────────────────

TEST(UuidGenerateTest, IsNotNull) {
	EXPECT_FALSE(Uuid::generate().is_null());
}

TEST(UuidGenerateTest, SetsVersion7) {
	for (int n = 0; n < 64; ++n) {
		Uuid u = Uuid::generate();
		EXPECT_EQ(u.bytes[6] >> 4, 0x7) << "attempt " << n;
	}
}

TEST(UuidGenerateTest, SetsRfc4122Variant) {
	for (int n = 0; n < 64; ++n) {
		Uuid u = Uuid::generate();
		EXPECT_EQ(u.bytes[8] >> 6, 0x2) << "attempt " << n;   // 0b10xxxxxx
	}
}

TEST(UuidGenerateTest, TimestampIsCurrent) {
	Uuid u = Uuid::generate();
	int64_t ts  = static_cast<int64_t>(timestamp_ms(u));
	int64_t now = now_ms();

	// Generous window: the call itself takes microseconds, but a loaded
	// CI machine may pause between the clock reads.
	EXPECT_GE(ts, now - 10000) << "timestamp " << ts << " vs now " << now;
	EXPECT_LE(ts, now + 10000) << "timestamp " << ts << " vs now " << now;
}

TEST(UuidGenerateTest, TimestampsAreNonDecreasing) {
	uint64_t prev = 0;
	for (int n = 0; n < 200; ++n) {
		uint64_t ts = timestamp_ms(Uuid::generate());
		EXPECT_GE(ts, prev) << "attempt " << n;
		prev = ts;
	}
}

TEST(UuidGenerateTest, GeneratesDistinctValues) {
	std::unordered_set<Uuid> seen;
	for (int n = 0; n < 500; ++n) seen.insert(Uuid::generate());
	EXPECT_EQ(seen.size(), 500u);
}

TEST(UuidGenerateTest, RandomTailDiffersBetweenCalls) {
	Uuid a = Uuid::generate();
	Uuid b = Uuid::generate();
	EXPECT_NE(a, b);

	// Bytes 9..15 are pure random payload; at least one must differ.
	bool tail_differs = false;
	for (int i = 9; i < 16; ++i) {
		if (a.bytes[i] != b.bytes[i]) tail_differs = true;
	}
	EXPECT_TRUE(tail_differs);
}

TEST(UuidGenerateTest, GeneratedRoundTripsThroughString) {
	for (int n = 0; n < 16; ++n) {
		Uuid u = Uuid::generate();
		EXPECT_EQ(Uuid::from_string(u.to_string()), u);
		EXPECT_EQ(Uuid::from_string(std::format("{}", u)), u);
	}
}
