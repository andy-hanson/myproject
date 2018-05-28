#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

inline uint32_t to_uint32(uint64_t u) {
	assert(u <= std::numeric_limits<uint32_t>::max());
	return static_cast<uint32_t>(u);
}
