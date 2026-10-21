#pragma once
#include <type_traits>
#include <limits>
#include <cmath>

namespace ledger {
	namespace __internal {
template<typename T>T sign(T val) {
    return val < 0 ? T(-1) : (val == T(0) ? T(0) : T(1));
}

template<typename T> requires std::is_signed_v<T> std::make_unsigned_t<T> abs(T n) {
	return n == std::numeric_limits<T>::min() ? static_cast<std::make_unsigned_t<T>>(std::numeric_limits<T>::max()) + 1 : static_cast<std::make_unsigned_t<T>>(abs(n));
}

template<typename T> requires std::is_unsigned_v<T> T abs(T n) {
	return n;
}

	}
}
