#version 300 es

in vec3 a_position;
in vec3 a_normal;
in uint a_material_id;

out vec3 frag_world_pos;
out vec3 frag_world_normal;
flat out uint frag_material_id;

uniform mat4 u_model;
uniform mat4 u_transform;

void main() {

	vec3 world_pos = (u_model * vec4(a_position, 1.0)).xyz;
	frag_world_pos = world_pos;
	frag_world_normal = normalize(vec3(u_model * vec4(a_normal, 0.0)));
	frag_material_id = a_material_id;

	gl_Position = u_transform * vec4(a_position, 1.0);
}
