#version 300 es

out lowp vec4 outColor;

in lowp vec3 frag_color;

void main() {
    outColor = vec4(frag_color * 0.1, 1.0);
}
