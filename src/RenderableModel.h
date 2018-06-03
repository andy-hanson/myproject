#pragma once

#include <glm/vec3.hpp>

#include "./util/DynArray.h"

struct VertexAttributesTri {
	glm::vec3 a_position;
	u32 a_material_id;
} __attribute__((packed));

struct VertexAttributesDot {
	glm::vec3 a_position;
	glm::vec3 a_normal;
	glm::vec3 a_color;
	u32 a_material_id;
} __attribute__((packed));

struct RenderableModel {
	DynArray<VertexAttributesTri> tris;
	DynArray<VertexAttributesDot> dots;
};
