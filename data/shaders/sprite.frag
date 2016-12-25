#version 330 core

in vec2 uv;
out vec4 out_color;

uniform sampler2DRect tex;
uniform float opacity = 1.0;

void
main()
{
	out_color = texture(tex, uv) * opacity;
}