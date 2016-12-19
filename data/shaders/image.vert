#version 330 core

uniform vec2 size;
uniform mat4 transform;

out vec2 uv;

const vec2 positions[4] = vec2[]
(
	vec2(0, 0),
	vec2(0, -1),
	vec2(1, 0),
	vec2(1, -1)
);

const vec2 uvs[4] = vec2[]
(
	vec2(0, 0),
	vec2(0, 1),
	vec2(1, 0),
	vec2(1, 1)
);

void
main()
{
	// compute vertex coordinate
	gl_Position = transform * vec4(positions[gl_VertexID] * size, 0, 1);

	// compute texture coordinate
	uv = uvs[gl_VertexID] * size;
}