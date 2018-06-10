#version 300 es

// TODO: PERF
precision highp float;

out vec4 outColor;

in vec3 frag_world_pos;
in vec3 frag_world_normal;
flat in uint frag_material_id;

// Note: must be kept in sync with c++ (and debug.vert)
const uint MAX_MATERIALS = 5u;
const uint MATERIAL_SIZE_FLOATS = 6u;

uniform mat4 u_model;
uniform mat4 u_transform;
uniform float u_materials[MAX_MATERIALS * MATERIAL_SIZE_FLOATS];

// This should match what's in c++
struct Material {
	vec3 diffuse;
	vec3 specular;
};

Material get_material() {
	uint i = MATERIAL_SIZE_FLOATS * (frag_material_id - 1u);
	return Material(
		vec3(u_materials[i + 0u], u_materials[i + 1u], u_materials[i + 2u]),
		vec3(u_materials[i + 3u], u_materials[i + 4u], u_materials[i + 5u]));
}

float Pi = 3.141592653589793238462643383279502884197169399375105820974944;

float cosEase(float x) {
	return -0.5 * cos(Pi * x) + 0.5;
}

float square(float x) {
	return x * x;
}
float power3(float x) {
	return square(x) * x;
}
float power4(float x) {
	return square(square(x));
}
float power5(float x) {
	return power4(x) * x;
}

// input and output are both fractions 0-1.
// Makes the input stick closer to 1.0.
float quartic_ease(float x) {
	return 1.0 - power4(x - 1.0);
}

float length2(vec3 v) {
	return square(v.x) + square(v.y) + square(v.z);
}

float distance2(vec3 a, vec3 b) {
	return length2(a - b);
}

vec3 direction_from_to(vec3 a, vec3 b) {
	return normalize(b - a);
}

// All vectors in this fn are in world coordinates.
vec3 lighting_for_light(vec3 world_pos, vec3 world_normal, Material material, vec3 light_pos) {
	world_pos = vec3(0.0); //TODO
	//world_normal = vec3(0.99, 0.0, 0.0);
	vec3 direction_to_light = direction_from_to(world_pos, light_pos);

	vec3 camera_pos = vec3(0.0, 0.0, -4.0); //TODO: don't hardcode

	float diffuse = dot(world_normal, direction_to_light);
	if (diffuse < 0.0) return vec3(0.0); // There definitely won't be a specular component.

	vec3 direction_to_camera = direction_from_to(world_pos, camera_pos);
	// reflect light by normal
	vec3 r = reflect(-direction_to_light, world_normal);
	float specular = dot(direction_to_camera, r);
	if (specular < 0.0) specular = 0.0;
	specular = power5(specular);

	vec3 light_color = vec3(20.0); //TODO
	float distance2 = distance2(world_pos, light_pos);
	return light_color * (diffuse * material.diffuse + specular * material.specular) / distance2;
}

vec3 calculate_lighting(vec3 world_pos, vec3 world_normal, Material material) {
	vec3 light_pos = vec3(-0.0, 4.0, 2.0);
	return lighting_for_light(world_pos, world_normal, material, light_pos);
}

void main() {
	outColor = vec4(calculate_lighting(frag_world_pos, frag_world_normal, get_material()), 1.0);
}
