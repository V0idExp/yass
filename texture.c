#include "error.h"
#include "memory.h"
#include "sprite.h"
#include "texture.h"
#include <assert.h>
#include <png.h>
#include <setjmp.h>
#include <stdlib.h>

static void*
read_image(const char *filename, unsigned int *r_width, unsigned int *r_height)
{
	assert(filename != NULL);

	void *data = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp rows = NULL;

	// open the given file
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		error(ERR_FILE_READ);
		return NULL;
	}

	// attempt to read 8 bytes and check whether we're reading a PNG file
	size_t hdr_size = 8;
	unsigned char hdr[hdr_size];
	if (fread(hdr, 1, hdr_size, fp) < hdr_size ||
	    png_sig_cmp(hdr, 0, hdr_size) != 0) {
		error(ERR_FILE_BAD);
		goto error;
	}

	// allocate libpng structs
	png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL,
		NULL,
		NULL
	);
	if (!png_ptr) {
		error(ERR_LIBPNG);
		goto error;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		error(ERR_LIBPNG);

	}

	// set the error handling longjmp point
	if (setjmp(png_jmpbuf(png_ptr))) {
		error(ERR_FILE_BAD);
		goto error;
	}

	// init file reading IO
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, hdr_size);

	// read image information
	png_read_info(png_ptr, info_ptr);

	// get image info
	int color_type = info_ptr->color_type;
	int bit_depth = info_ptr->bit_depth;

	// transform paletted images to RGB
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}

	// transform packed grayscale images to 8bit
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_gray_1_2_4_to_8(png_ptr);
	} else if (color_type == PNG_COLOR_TYPE_GRAY ||
	           color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	// strip 16bit images down to 8bit
	else if (bit_depth == 16) {
		png_set_strip_16(png_ptr);
	}
	// expand 1-byte packed pixels
	else if (bit_depth < 8) {
		png_set_packing(png_ptr);
	}

	// add full alpha channel
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png_ptr);
	}

	// retrieve image size
	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);

	if (r_width) {
		*r_width = width;
	}
	if (r_height) {
		*r_height = height;
	}

	// allocate space for image data
	size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	data = malloc(height * rowbytes);
	if (!data) {
		error(ERR_NO_MEM);
		goto error;
	}

	// setup an array of image row pointers
	rows = malloc(height * sizeof(png_bytep));
	if (!rows) {
		error(ERR_NO_MEM);
		goto error;
	}
	for (size_t r = 0; r < height; r++) {
		rows[r] = data + rowbytes * r;
	}

	// read image data
	png_read_image(png_ptr, rows);

cleanup:
	free(rows);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return data;

error:
	free(data);
	data = NULL;
	goto cleanup;
}

struct Texture*
texture_from_file(const char *filename)
{
	assert(filename != NULL);

	// allocate texture struct
	struct Texture *texture = make(struct Texture);
	if (!texture) {
		return NULL;
	}

	// read PNG image
	void *image_data = read_image(
		filename,
		&texture->width,
		&texture->height
	);
	if (!image_data) {
		goto error;
	}

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
		texture->width,
		texture->height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data
	);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (glGetError() != GL_NO_ERROR || !texture->hnd) {
		error(ERR_OPENGL);
		goto error;
	}

cleanup:
	free(image_data);
	return texture;

error:
	texture_destroy(texture);
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