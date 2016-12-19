#version 330 core

in vec2 uv;
out vec4 out_color;

uniform vec2 size;
uniform uvec4 border;
uniform sampler2DRect tex;

float
clamp_coord(float v, float size, float texSize, uint lo, uint hi)
{
	if (v > lo) {
		if (v >= size - hi) {
			v -= (size - hi);
			v += texSize - hi;
		} else {
			v -= lo;
			v = lo + uint(v) % uint(texSize - lo - hi);
		}
	}
	return v;
}

void
main()
{
	ivec2 texSize = textureSize(tex);
	vec2 norm_uv = vec2(
		clamp_coord(uv.x, size.x, texSize.x, border.r, border.g),
		clamp_coord(uv.y, size.y, texSize.y, border.b, border.a)
	);
	out_color = texture(tex, norm_uv);
}