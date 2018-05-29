#version 300 es

in vec3 position;
in vec3 color;

out vec3 frag_color;

uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * vec4(position, 1.0);
    frag_color = color;

    gl_PointSize = 50.0;
}
