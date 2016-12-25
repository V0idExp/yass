#version 330 core

uniform sampler2DRect atlas_tex;
uniform uint atlas_offset;
uniform vec4 color;

flat in uint char;
in vec2 uv;
out vec4 out_color;

void main()
{
	float s = uv.s + char * atlas_offset;
	float t = uv.t;
	out_color = color * vec4(texture(atlas_tex, vec2(s, t)).r);
}
