#version 300 es

out lowp vec4 outColor;

in lowp vec3 frag_color;
flat in uint frag_material_id;

uniform highp usampler2D u_depth_texture;

void main() {
	// Note: gl_FragCoord.x ranges from 0..viewport width. Similar for y.
	// But sampling a texture should sample in 0..1.
	//TODO: don't hardcode viewport size
	lowp vec2 sample_at_coord = gl_FragCoord.xy / vec2(1024, 1024);
	uvec4 value_in_texture = texture(u_depth_texture, sample_at_coord);

	if (value_in_texture.r != frag_material_id) discard;

	// gl_PointCoord coordinates are 0..1. Convert to -1..1.
	lowp vec2 normal_pointCoord = gl_PointCoord * 2.0 - vec2(1.0, 1.0);
	// Draw a circle, so discard outside radius.
	if (length(normal_pointCoord) > 1.0) discard;

	outColor = vec4(frag_color, 1.0); 
}
