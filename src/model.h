#pragma once

#include <glm/vec3.hpp>

#include "./util/DynArray.h"
#include "./util/int.h"

//Note: texture and vertex indices were parsed away from 1-based. So this is 1 less than what's in the file.

struct Face {
	u8 material;
	u8 vertex_0;
	u8 vertex_1;
	u8 vertex_2;
	u8 normal_0;
	u8 normal_1;
	u8 normal_2;
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

// Note: every member must be a float, since we pass this to glVertexAttribPointer.
// TODO: we'll probably want separate attributes for flat vs dots rendering
//If changing this, must also change the call to `glVertexAttribPointer`
struct VertexAttributes {
	glm::vec3 pos;
	glm::vec3 color;
	//glm::vec2 texture_coords;
} __attribute__((packed));

struct RenderableModel {
	DynArray<VertexAttributes> tris;
	DynArray<VertexAttributes> dots;
};
