#pragma once

#include <glm/vec3.hpp>

#include "./util/DynArray.h"

// Must be kept in sync with `dot.vert`.
struct Material {
	glm::vec3 diffuse;
	glm::vec3 specular;
} __attribute__((packed));

struct VertexAttributesTri {
	glm::vec3 a_position;
	u32 a_material_id;
} __attribute__((packed));

struct VertexAttributesDot {
	glm::vec3 a_position;
	glm::vec3 a_normal;
	u32 a_material_id;
} __attribute__((packed));

struct RenderableModel {
	DynArray<VertexAttributesTri> tris;
	DynArray<VertexAttributesDot> dots;
};
