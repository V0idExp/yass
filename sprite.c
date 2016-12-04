#include "error.h"
#include "memory.h"
#include "sprite.h"
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>
#include <stdio.h>

static GLuint
texture_from_file(const char *filename, int *width, int *height)
{
	assert(width != NULL);
	assert(height != NULL);
	*width = *height = 0;
	GLuint texture = 0;

	// initialize SDL image
	static int img_initialized = 0;
	if (!img_initialized && IMG_Init(0) == -1) {
		fprintf(
			stderr,
			"failed to initialize SDL image library: %s\n",
			IMG_GetError()
		);
		error(ERR_SDL);
		return 0;
	}
	img_initialized = 1;

	// create a SDL surface from given image file
	SDL_Surface *surf = IMG_Load(filename);
	if (!surf) {
		fprintf(
			stderr,
			"failed to load image `%s`: %s\n", filename,
			IMG_GetError()
		);
		error(ERR_FILE_READ);
		return 0;
	}

	// convert the surface to RGBA8888 format
#if SDL_COMPILEDVERSION >= SDL_VERSIONNUM(2, 0, 5)
	SDL_Surface *rgba_surf = SDL_ConvertSurfaceFormat(
		surf,
		SDL_PIXELFORMAT_RGBA32,
		0
	);
#else
	SDL_Surface *rgba_surf = SDL_ConvertSurfaceFormat(
		surf,
		SDL_BYTEORDER == SDL_BIG_ENDIAN ?
		SDL_PIXELFORMAT_RGBA8888 :
		SDL_PIXELFORMAT_ARGB8888,
		0
	);
#endif
	if (!rgba_surf) {
		fprintf(
			stderr,
			"image conversion to RGBA32 format failed: %s\n",
			SDL_GetError()
		);
		error(ERR_SDL);
		goto error;
	}

	// create OpenGL texture
	glGenTextures(1, &texture);
	if (!texture) {
		fprintf(
			stderr,
			"OpenGL texture creation failed (error %d)\n",
			glGetError()
		);
		error(ERR_OPENGL);
		goto error;
	}

	// initialize texture data
	glBindTexture(GL_TEXTURE_RECTANGLE, texture);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	SDL_LockSurface(rgba_surf);
	glTexImage2D(
		GL_TEXTURE_RECTANGLE,
		0,
		GL_RGBA8,
		rgba_surf->w,
		rgba_surf->h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		rgba_surf->pixels
	);
	SDL_UnlockSurface(rgba_surf);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"OpenGL texture initialization failed (error %d)\n",
			glGetError()
		);
		error(ERR_OPENGL);
		goto error;
	}

	*width = rgba_surf->w;
	*height = rgba_surf->h;

cleanup:
	SDL_FreeSurface(rgba_surf);
	SDL_FreeSurface(surf);
	return texture;

error:
	glDeleteTextures(1, &texture);
	texture = 0;
	goto cleanup;
}

struct Sprite*
sprite_from_file(const char *filename)
{
	struct Sprite *spr = make(struct Sprite);
	if (!spr) {
		return NULL;
	}

	spr->texture = texture_from_file(filename, &spr->width, &spr->height);
	if (!spr->texture) {
		goto error;
	}

	glGenVertexArrays(1, &spr->vao);
	if (!spr->vao) {
		fprintf(
			stderr,
			"failed to generate OpenGL sprite VAO (error %d)\n",
			glGetError()
		);
		error(ERR_OPENGL);
		goto error;
	}

	return spr;

error:
	sprite_destroy(spr);
	return NULL;
}

void
sprite_destroy(struct Sprite *spr)
{
	if (spr) {
		glDeleteVertexArrays(1, &spr->vao);
		glDeleteTextures(1, &spr->texture);
		free(spr);
	}
}