#pragma once

struct Texture {
	GLuint hnd;
	unsigned width, height;
};

struct Texture*
texture_from_file(const char *filename);

void
texture_destroy(struct Texture *texture);