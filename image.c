#include "error.h"
#include "image.h"
#include "memory.h"

struct Image*
image_new(void)
{
	struct Image *image = make(struct Image);
	if (!image) {
		return NULL;
	}

	glGenVertexArrays(1, &image->vao);
	if (glGetError() != GL_NO_ERROR || !image->vao) {
		error(ERR_OPENGL);
		image_destroy(image);
		return NULL;
	}

	return image;
}

void
image_destroy(struct Image *image)
{
	if (image) {
		glDeleteVertexArrays(1, &image->vao);
		destroy(image);
	}
}