#include "error.h"
#include "memory.h"
#include "sprite.h"
#include "texture.h"
#include <assert.h>

struct Sprite*
sprite_from_file(const char *filename)
{
	assert(filename != NULL);

	// create an empty sprite struct
	struct Sprite *spr = make(struct Sprite);
	if (!spr) {
		return NULL;
	}

	// load the texture from image file
	spr->texture = texture_from_file(filename);
	if (!spr->texture) {
		sprite_destroy(spr);
		return NULL;
	}
	spr->width = spr->texture->width;
	spr->height = spr->texture->height;

	// generate a VAO for the sprite
	glGenVertexArrays(1, &spr->vao);
	if (glGetError() != GL_NO_ERROR || !spr->vao) {
		error(ERR_OPENGL);
		sprite_destroy(spr);
		return NULL;
	}

	return spr;
}

void
sprite_destroy(struct Sprite *spr)
{
	if (spr) {
		glDeleteVertexArrays(1, &spr->vao);
		texture_destroy(spr->texture);
		destroy(spr);
	}
}