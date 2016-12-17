#pragma once

#include <GL/glew.h>
#include <stddef.h>

struct Widget {
	GLuint vao;
	struct Texture *texture;
	unsigned int width, height;
	struct {
		uint8_t left, right;
		uint8_t top, bottom;
	} border;
};

struct Widget*
widget_new(void);

void
widget_destroy(struct Widget *widget);