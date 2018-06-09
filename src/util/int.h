#pragma once

#include <cstdint>
#include <limits>

#include "./assert.h"

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using uint = unsigned;

inline u32 i64_to_u32(i64 l) {
	check(l >= 0 && l <= std::numeric_limits<u32>::max());
	return static_cast<u32>(l);
}
inline u32 u64_to_u32(u64 u) {
	check(u <= std::numeric_limits<u32>::max());
	return static_cast<u32>(u);
}
inline u32 ulong_to_u32(unsigned long u) {
	check(u <= std::numeric_limits<u32>::max());
	return static_cast<u32>(u);
}
inline u8 u64_to_u8(u64 u) {
	check(u <= std::numeric_limits<u8>::max());
	return static_cast<u8>(u);
}
inline u8 u32_to_u8(u32 u) {
	check(u <= std::numeric_limits<u8>::max());
	return static_cast<u8>(u);
}

inline uint int_to_uint(int i) {
	check(i >= 0);
	return static_cast<uint>(i);
}
inline u8 int_to_u8(int i) {
	check(i >= 0 && i < std::numeric_limits<u8>::max());
	return static_cast<u8>(i);
}
inline unsigned long long_to_ulong(long l) {
	check(l >= 0 && l <= std::numeric_limits<uint>::max());
	return static_cast<uint>(l);
}
inline int long_to_int(long l) {
	check(l >= std::numeric_limits<int>::min() && l <= std::numeric_limits<int>::max());
	return static_cast<int>(l);
}

inline u8 safe_incr_u8(u8 u) {
	check(u != std::numeric_limits<u8>::max());
	return u + 1;
}

inline u64 i64_to_u64(i64 i) {
	check(i >= 0);
	return static_cast<u64>(i);
}

inline int uint_to_int(uint u) {
	check(u <= std::numeric_limits<int>::max());
	return static_cast<int>(u);
}
inline int ulong_to_int(unsigned long u) {
	check(u <= std::numeric_limits<int>::max());
	return static_cast<int>(u);
}

inline u8 char_to_u8(char c) {
	check(c >= 0);
	return static_cast<u8>(c);
}

inline u32 safe_div(u32 a, u32 b) {
	check(b != 0 && a % b == 0);
	return a / b;
}
inline u32 safe_mul(u32 a, u32 b) {
	if (b == 0) return 0;
	check((std::numeric_limits<u32>::max() / b) > a);
	return a *b;
}

inline bool int_to_bool(int i) {
	check(i == 0 || i == 1);
	return i == 1;
}
