#pragma once
#include <type_traits>
#include <limits>
#include <cmath>

namespace ledger {
	namespace __internal {
template<typename T> inline T sign(T val) {
    return val < 0 ? T(-1) : (val == T(0) ? T(0) : T(1));
}

template<typename T> requires std::is_signed_v<T> inline std::make_unsigned_t<T> abs(T n) {
	using U = std::make_unsigned_t<T>;
	if (n == std::numeric_limits<T>::min()) {
		return static_cast<U>(std::numeric_limits<T>::max()) + U{1};
	}
	return n < 0 ? static_cast<U>(-n) : static_cast<U>(n);
}

template<typename T> requires std::is_unsigned_v<T> T abs(T n) {
	return n;
}

inline char hex_char(uint8_t half_byte, bool upper) {
	static constexpr char hex_upper[] = "0123456789ABCDEF";
	static constexpr char hex_lower[] = "0123456789abcdef";
	return upper ? hex_upper[half_byte] : hex_lower[half_byte];
}
	}
}
