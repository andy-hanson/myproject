#version 300 es

// TODO: PERF
precision highp float;

out vec4 outColor;

in vec3 frag_color;
in float frag_point_size;
flat in uint frag_material_id;

uniform highp usampler2D u_material_id_texture;

//TODO: don't hardcode
const uint VIEWPORT_WIDTH = 1024u;
const uint VIEWPORT_HEIGHT = 1024u;

uint material_id_at_pixel(uint x, uint y) {
	return texture(u_material_id_texture, vec2(x, y) / vec2(VIEWPORT_WIDTH, VIEWPORT_HEIGHT)).r;
}

float bool_to_float(bool b) {
	return b ? 1.0 : 0.0;
}

float texture_sample_fraction() {
	float frag_x = gl_FragCoord.x;
	float frag_y = gl_FragCoord.y;
	float lo_x_f = floor(frag_x);
	uint lo_x = uint(lo_x_f);
	uint hi_x = uint(ceil(frag_x));
	float lo_y_f = floor(frag_y);
	uint lo_y = uint(lo_y_f);
	uint hi_y = uint(ceil(frag_y));

	// Note: gl_FragCoord.x ranges from 0..viewport width. Similar for y.
	// But sampling a texture should sample in 0..1.
	bool dl = material_id_at_pixel(lo_x, lo_y) == frag_material_id;
	bool dr = material_id_at_pixel(hi_x, lo_y) == frag_material_id;
	bool ul = material_id_at_pixel(lo_x, hi_y) == frag_material_id;
	bool ur = material_id_at_pixel(hi_x, hi_y) == frag_material_id;

	if (dl && dr && ul && ur) return 1.0;
	if (!dl && !dr && !ul && !ur) return 0.0;

	float right_frac = frag_x - lo_x_f;
	float up_frac = frag_y - lo_y_f;
	float left_frac = 1.0 - right_frac;
	float bottom_frac = 1.0 - up_frac;

	float dl_frac = left_frac * bottom_frac;
	float dr_frac = right_frac * bottom_frac;
	float ul_frac = left_frac * up_frac;
	float ur_frac = right_frac * up_frac;

	return (
		bool_to_float(dl) * dl_frac +
		bool_to_float(dr) * dr_frac +
		bool_to_float(ul) * ul_frac +
		bool_to_float(ur) * ur_frac);
}

float texture_sample_fraction_simple() {
	return bool_to_float(material_id_at_pixel(uint(round(gl_FragCoord.x)), uint(round(gl_FragCoord.y))) == frag_material_id);
}

// Fn that is 1 from 0..(1 - dieoff), then goes down to 0 linearly.
float dieoff(float f, float dieoff) {
	float min = 1.0 - dieoff;
	return f < min ? 1.0 : 1.0 - ((f - min) / dieoff);
}

void main() {
	// gl_PointCoord coordinates are 0..1. Convert to -1..1.
	vec2 normal_pointCoord = gl_PointCoord * 2.0 - vec2(1.0, 1.0);

	// Outer 2 pixels fade out.
	float radius_dieoff = 2.0 / frag_point_size;

	float radius = length(normal_pointCoord);
	// Draw a circle, so discard outside radius.
	if (radius > 1.0) discard;

	float strength = texture_sample_fraction();
	if (strength == 0.0) discard;

	outColor = vec4(frag_color, strength * dieoff(radius, radius_dieoff));
}
