#pragma once

#include <GL/glew.h>
#include <stddef.h>

struct Image {
	GLuint vao;
	struct Texture *texture;
	unsigned int width, height;
	struct {
		uint8_t left, right;
		uint8_t top, bottom;
	} border;
};

struct Image*
image_new(void);

void
image_destroy(struct Image *image);