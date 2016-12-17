#version 330 core

in vec2 uv;
out vec4 out_color;

uniform vec2 size;
uniform uvec4 border;
uniform sampler2DRect tex;

float
clamp_coord(float v, float size, uint low, uint high)
{
	uint middle = uint(size) - high;
	if (v > low && v < size - high) {
		return float(uint(v) % size);
	} else if (v >= size - high) {
		return size - v;
	}
	return v;
}

void
main()
{
	vec2 norm_uv = vec2(
		clamp_coord(uv.x, size.x, border.r, border.g),
		clamp_coord(uv.y, size.y, border.b, border.a)
	);
	out_color = texture(tex, norm_uv);
}