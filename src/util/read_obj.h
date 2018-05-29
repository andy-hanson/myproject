#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "./DynArray.h"
#include "./int.h"

//Note: texture and vertex indices were parsed away from 1-based. So this is 1 less than what's in the file.

struct Face {
	uint8_t material;
	uint8_t vertex_0;
	uint8_t vertex_1;
	uint8_t vertex_2;
	uint8_t normal_0;
	uint8_t normal_1;
	uint8_t normal_2;
};

//TODO:MOVE
inline bool is_fraction(float f) {
	return f >= 0 && f <= 1;
}

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

struct Material {
	float ns;
	Color ka;
	Color kd;
	Color ks;
	Color ke;
	float ni;
	float d;
	u8 illum;
};

struct Model {
	DynArray<Material> materials;
	DynArray<glm::vec3> vertices;
	DynArray<glm::vec3> normals;
	DynArray<Face> faces;
};

Model parse_model(const char* mtl_source, const char* obj_source);
