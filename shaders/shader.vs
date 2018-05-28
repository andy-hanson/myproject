#version 300 es

in vec2 position;
in vec3 color;
in vec2 texCoord;

out vec3 frag_color;
out vec2 frag_texCoord;

// NOTE TO SELF: Can't use `varying` in opengl ES 3.0.
// Use 'out' in the vertex shader and 'in' in the fragment shader.

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    frag_color = color;
    frag_texCoord = texCoord;
}
