#pragma once

#include <cmath>

inline float float_sin(float f) {
	return static_cast<float>(sin(static_cast<double>(f)));
}
