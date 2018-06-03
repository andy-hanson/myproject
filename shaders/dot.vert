#version 300 es

in vec3 a_position;
in vec3 a_normal;
in uint a_material_id;

out vec3 frag_color;
flat out uint frag_material_id;

// Note: must be kept in sync with c++
const uint MAX_MATERIALS = 5u;
const uint MATERIAL_SIZE_FLOATS = 6u;

uniform mat4 u_model;
uniform mat4 u_transform;
uniform float u_materials[MAX_MATERIALS * MATERIAL_SIZE_FLOATS];
// There's also u_depth_texture used by dot.frag

// This should match what's in c++
struct Material {
	vec3 diffuse;
	vec3 specular;
};

Material get_material() {
	uint i = MATERIAL_SIZE_FLOATS * (a_material_id - 1u);
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

vec3 ignore(vec3 v) {
	return v * 0.001;
}

// input and output are both fractions 0-1.
// Makes the input stick closer to 1.0.
float quartic_ease(float x) {
	return 1.0 - power4(x - 1.0);
}

void discardVertex() {
	gl_PointSize = 0.0;
	gl_Position = vec4(-100, -100, -100, 1);
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
	vec3 light_pos = vec3(-0.0, 4.0, -2.0);
	return lighting_for_light(world_pos, world_normal, material, light_pos);
}

void main() {
	lowp vec3 screen_normal = normalize(vec3(u_transform * vec4(a_normal, 0.0)));

	Material material = get_material();

	if (screen_normal.z > 0.0) {
		// Facing away from the camera
		discardVertex();
		return;
	}

	vec3 world_pos = (u_model * vec4(a_position, 1.0)).xyz;
	vec3 world_normal = normalize(vec3(u_model * vec4(a_normal, 0.0)));
	vec3 lit_color = calculate_lighting(world_pos, world_normal, material);

	// As screen_normal approaches 0, get smaller.
	gl_Position = u_transform * vec4(a_position, 1.0);
	gl_PointSize = 100.0 * quartic_ease(-screen_normal.z);

	frag_color = lit_color;
	frag_material_id = a_material_id;
}
