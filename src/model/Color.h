#pragma once

//TODO:MOVE
inline bool is_fraction(float f) {
	return f >= 0 && f <= 1;
}

//TODO:MOVE
struct Color {
	float r;
	float g;
	float b;

	inline Color(float _r, float _g, float _b) : r{_r}, g{_g}, b{_b} {
		assert(is_fraction(r) && is_fraction(g) && is_fraction(b));
	}
	explicit Color(glm::vec3 v) : Color{v.r, v.g, v.b} {}

	inline glm::vec3 vec3() const {
		return { r, g, b };
	}
};
