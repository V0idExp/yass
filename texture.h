#pragma once

struct Texture {
	GLuint hnd;
	unsigned width, height;
	struct Border {
		uint8_t left, right;
		uint8_t top, bottom;
	} border;
};

struct Texture*
texture_from_file(const char *filename);

void
texture_destroy(struct Texture *texture);