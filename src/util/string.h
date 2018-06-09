#pragma once

#include <string>
#include "./Slice.h"

inline Slice<char> to_slice(const std::string& s) {
	return Slice<char> { s.begin().base(), ulong_to_u32(s.size()) };
}

