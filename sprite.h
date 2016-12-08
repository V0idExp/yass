#pragma once

#include <GL/glew.h>

struct Sprite {
	GLuint vao;
	struct Texture *texture;
	int width, height;
};

struct Sprite*
sprite_from_file(const char *filename);

void
sprite_destroy(struct Sprite *spr);