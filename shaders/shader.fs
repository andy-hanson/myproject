#version 300 es

out lowp vec4 outColor;

//uniform lowp vec3 u_triangleColor;
//uniform lowp sampler2D tex;

in lowp vec3 frag_color;
//in lowp vec2 frag_texCoord;

void main() {
	//lowp vec4 tex_color = texture(tex, frag_texCoord);
    outColor = vec4(frag_color.r, frag_color.g, frag_color.b, 1.0);
}
