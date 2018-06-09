#pragma once

#include <glm/vec2.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
	glm::vec3 position;
	glm::quat quat;
};
