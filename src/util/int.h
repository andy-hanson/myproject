#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

inline u32 i64_to_u32(int64_t l) {
	assert(l >= 0 && l <= std::numeric_limits<uint32_t>::max());
	return static_cast<uint32_t>(l);
}
inline u32 u64_to_u32(uint64_t u) {
	assert(u <= std::numeric_limits<uint32_t>::max());
	return static_cast<uint32_t>(u);
}
inline u8 uint_to_u8(uint u) {
	assert(u <= std::numeric_limits<uint8_t>::max());
	return static_cast<u8>(u);
}

inline uint int_to_uint(int i) {
	assert(i >= 0);
	return static_cast<uint>(i);
}
inline uint long_to_uint(long l) {
	assert(l >= 0 && l <= std::numeric_limits<uint>::max());
	return static_cast<uint>(l);
}

inline u64 i64_to_u64(i64 i) {
	assert(i >= 0);
	return static_cast<u64>(i);
}

inline int uint_to_int(uint u) {
	assert(u <= std::numeric_limits<int>::max());
	return static_cast<int>(u);
}

inline u32 safe_div(u32 a, u32 b) {
	assert(b != 0);
	assert(a % b == 0);
	return a / b;
}
