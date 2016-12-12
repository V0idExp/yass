#version 330 core

in vec2 uv;
out vec4 out_color;

uniform vec2 size;
uniform uvec4 border;
uniform sampler2DRect tex;

void
main()
{
	uint left = border.r;
	uint right = border.g;
	uint middle = right - left;
	vec2 norm_uv = uv;
	if (uv.x > left && uv.x < size.x - right) {
		norm_uv.x = left + uint(uv.x) % middle;
	} else if (uv.x > left + middle && uv.x >= size.x - right) {
		norm_uv.x = size.x - uv.x;
	}
	out_color = texture(tex, norm_uv);
}