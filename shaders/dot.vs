#version 300 es

in vec3 a_position;
in vec3 a_normal;
in vec3 a_color;
in uint a_material_id;

out vec3 frag_color;
flat out uint frag_material_id;

//uniform sampler2D depth_texture;
uniform mat4 u_model;
uniform mat4 u_view_model;
uniform mat4 u_transform;

float Pi = 3.141592653589793238462643383279502884197169399375105820974944;

float cosEase(float x) {
	return -0.5 * cos(Pi * x) + 0.5;
}

float square(float x) {
	return x * x;
}

// input and output are both fractions 0-1.
// Makes the input stick closer to 1.0.
float quartic_ease(float x) {
	return 1.0 - square(square(x - 1.0));
}

void discardVertex() {
	gl_PointSize = 0.0;
	gl_Position = vec4(-100, -100, -100, 1);
}

void main() {
	gl_Position = u_transform * vec4(a_position, 1.0);

	lowp vec3 ignore = normalize(vec3(u_model * vec4(a_normal, 0.0))) +  normalize(vec3(u_view_model * vec4(a_normal, 0.0)));;//TODO:KILL
	lowp vec3 screen_normal = normalize(vec3(u_transform * vec4(a_normal, 0.0)));
	
	if (screen_normal.z > 0.0) {
		// Facing away from the camera
		discardVertex();
		return;
	}

	// As screen_normal approaches 0, get smaller.
	gl_PointSize = 100.0 * quartic_ease(-screen_normal.z);

	frag_color = ignore * 0.001 + a_color;
	frag_material_id = a_material_id;
}
