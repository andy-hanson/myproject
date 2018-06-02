#pragma once

#include <glm/vec3.hpp>

#include "./util/DynArray.h"

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
