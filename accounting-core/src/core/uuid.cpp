#include "ledger/core/uuid.hpp"
#include <random>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <functional>

using namespace std;
using namespace std::chrono;

namespace ledger {

Uuid Uuid::generate() {
	// UUIDv7: first 6 bytes = Unix timestamp in milliseconds, last 10 bytes = random
	Uuid uuid{};

	// Fill all 16 bytes with random data
	random_device rd;
	mt19937_64 gen(rd());
	uniform_int_distribution<uint64_t> dist;
	uint64_t x[2] = {dist(gen), dist(gen)};
	memcpy(uuid.bytes, &x, sizeof(x));

	// Overwrite first 6 bytes with timestamp (big-endian)
	auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	auto p = uuid.bytes.data();
	for (int i = 5; i >= 0; --i) {
		p[i] = static_cast<uint8_t>(ms & 0xFF);
		ms >>= 8;
	}

	// Set version bits (version = 7) at byte 6 high nibble
	p[6] = (p[6] & 0x0F) | 0x70;
	// Set variant bits (variant = 10xx) at byte 8 high 2 bits
	p[8] = (p[8] & 0x3F) | 0x80;

	return uuid;
}

Uuid Uuid::from_string(string_view sv) {
	// Lenient: extract first 32 hex chars, ignore dashes/braces/spaces/etc.
	Uuid uuid{};
	int i = 0;
	uint8_t hi;
	for (auto c : sv) {
		uint8_t half_byte;
		if (c >= '0' && c <= '9') half_byte = static_cast<uint8_t>(c - '0');
		else if (c >= 'a' && c <= 'f') half_byte = static_cast<uint8_t>(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F') half_byte = static_cast<uint8_t>(c - 'A' + 10);
		else continue;
		if (i & 1) {
			uuid.bytes[i>>1] = hi | half_byte;
		} else {
			hi = half_byte << 4;
		}
		if (++i == 32) break;
	}
	if (i < 32) throw invalid_argument("Invalid UUID string");
	return uuid;
}

string Uuid::to_string(bool upper) const {
	string result('-', 36);
	auto pos = result.begin();
	for (size_t i = 0; i < 16; ++i) {
		if (i == 4 || i == 6 || i == 8 || i == 10) ++pos;
		*pos++ = __internal::hex_char(bytes[i] >> 4, upper);
		*pos++ = __internal::hex_char(bytes[i] & 0x0F, upper);
	}
	return result;
}

bool Uuid::is_null() const {
	uint64_t lo, hi;
	memcpy(&lo, bytes.data(), 8);
	memcpy(&hi, bytes.data() + 8, 8);
	return lo == 0 && hi == 0;
}

} // namespace ledger

size_t std::hash<ledger::Uuid>::operator()(ledger::Uuid const& u) const {
	uint64_t h = 0xcbf29ce484222325ULL;
	for (auto b : u.bytes) {
		h ^= b;
		h *= 0x100000001b3ULL;
	}
	return static_cast<size_t>(h);
}
