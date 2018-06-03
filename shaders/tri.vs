#version 300 es

in vec3 a_position;
in uint a_material_id;

flat out uint frag_material_id;

uniform mat4 u_transform;

void main() {
	gl_Position = u_transform * vec4(a_position, 1.0);
	frag_material_id = a_material_id;
}
