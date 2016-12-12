#include "error.h"
#include "memory.h"
#include "sprite.h"
#include "texture.h"
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>

struct Texture*
texture_from_file(const char *filename)
{
	assert(filename != NULL);
	SDL_Surface *surf = NULL, *rgba_surf = NULL;
	struct Texture *texture = make(struct Texture);
	if (!texture) {
		return NULL;
	}

	// initialize SDL image
	static int img_initialized = 0;
	if (!img_initialized && IMG_Init(0) == -1) {
		error(ERR_SDL);
		goto error;
	}
	img_initialized = 1;

	// create a SDL surface from given image file
	surf = IMG_Load(filename);
	if (!surf) {
		error(ERR_FILE_READ);
		goto error;
	}

	// convert the surface to RGBA8888 format
#if SDL_COMPILEDVERSION >= SDL_VERSIONNUM(2, 0, 5)
	rgba_surf = SDL_ConvertSurfaceFormat(
		surf,
		SDL_PIXELFORMAT_RGBA32,
		0
	);
#else
	rgba_surf = SDL_ConvertSurfaceFormat(
		surf,
		SDL_BYTEORDER == SDL_BIG_ENDIAN ?
		SDL_PIXELFORMAT_RGBA8888 :
		SDL_PIXELFORMAT_ARGB8888,
		0
	);
#endif
	if (!rgba_surf) {
		error(ERR_SDL);
		goto error;
	}
	texture->width = rgba_surf->w;
	texture->height = rgba_surf->h;

	// create and initialize OpenGL texture
	glGenTextures(1, &texture->hnd);
	glBindTexture(GL_TEXTURE_RECTANGLE, texture->hnd);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (glGetError() != GL_NO_ERROR || !texture->hnd) {
		error(ERR_OPENGL);
		goto error;
	}

cleanup:
	SDL_FreeSurface(rgba_surf);
	SDL_FreeSurface(surf);
	return texture;

error:
	destroy(texture);
	texture = NULL;
	goto cleanup;
}

void
texture_destroy(struct Texture *texture)
{
	if (texture) {
		glDeleteTextures(1, &texture->hnd);
		destroy(texture);
	}
}