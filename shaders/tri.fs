#version 300 es

// writes to a texture
layout(location = 0) out uvec4 material_id;
//layout(location = 0) out lowp vec4 material_id;

flat in uint frag_material_id;

void main() {
	/*
	DEBUG VIEW: (must also change out material_id)
	if (frag_material_id == 1u) {
		material_id = vec4(1.0, 0.0, 1.0, 1.0);
	} else if (frag_material_id == 2u) {
		material_id = vec4(0.0, 1.0, 0.0, 1.0);
	} else {
		material_id = vec4(0.0, 1.0, 1.0, 1.0);
	}*/

	// https://stackoverflow.com/questions/10563097/glsl-fragmentshader-render-objectid#20706606
	// Output is GL_R32UI
	material_id = uvec4(frag_material_id, 0, 0, 0);
}
