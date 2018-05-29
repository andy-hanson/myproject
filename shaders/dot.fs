#version 300 es

out lowp vec4 outColor;

in lowp vec3 frag_color;

void main() {
	// gl_PointCOord coordinates are 0..1. Convert to -1..1.
	lowp vec2 normal_pointCoord = gl_PointCoord * 2.0 - vec2(1.0, 1.0);
	if (length(normal_pointCoord) > 1.0) discard;
    outColor = vec4(frag_color, 1.0);
}
