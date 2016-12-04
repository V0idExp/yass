#include "font.h"
#include "text.h"
#include "strutils.h"
#include <GL/glew.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Text*
text_new(struct Font *font)
{
	assert(font != NULL);

	struct Text *text = malloc(sizeof(struct Text));
	if (!text) {
		return NULL;
	}
	text->font = font;
	text->len = 0;
	text->width = 0;
	text->height = 0;
	text->string = NULL;
	text->vao = text->coords = text->chars = 0;

	// generate vertex array
	glGenVertexArrays(1, &text->vao);
	if (!text->vao) {
		fprintf(
			stderr,
			"failed to initialize text object vertex array "
			"(OpenGL error %d)\n",
			glGetError()
		);
		goto error;
	}
	glBindVertexArray(text->vao);

	// generate vertex buffers
	// NOTE: buffers are initialized when `text_set_string()` is called
	glGenBuffers(1, &text->coords);
	glGenBuffers(1, &text->chars);
	if (!text->coords || !text->chars) {
		fprintf(
			stderr,
			"failed to initialize text object buffers "
			"(OpenGL error %d)\n",
			glGetError()
		);
		goto error;
	}

	// setup coords attribute array
	glBindBuffer(GL_ARRAY_BUFFER, text->coords);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glVertexAttribDivisor(0, 1);

#ifdef DEBUG
	GLenum gl_err = glGetError();
	if (gl_err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"failed to text coords attribute "
			"(OpenGL error %d)\n",
			gl_err
		);
		goto error;
	}
#endif

	// setup character indices attribute array
	// NOTE: each index is one byte long (the character itself), thus,
	// anything except ASCII is not supported
	glBindBuffer(GL_ARRAY_BUFFER, text->chars);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, 0, (GLvoid*)0);
	glVertexAttribDivisor(1, 1);

#ifdef DEBUG
	gl_err = glGetError();
	if (gl_err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"failed to text chars attribute "
			"(OpenGL error %d)\n",
			gl_err
		);
		goto error;
	}
#endif

cleanup:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return text;

error:
	text_destroy(text);
	text = NULL;
	goto cleanup;
}

int
text_set_string(struct Text *text, const char *str)
{
	assert(text != NULL);
	assert(str != NULL);

	size_t len = strlen(str);

	// do not change the string if it's equal to internal copy
	if (text->string &&
	    len == text->len &&
	    strcmp(text->string, str) == 0) {
		return 1;
	}

	// make an internal copy of the string
	free(text->string);
	if (!(text->string = string_copy(str))) {
		return 0;
	}
	text->len = len;

	// setup the buffer of character indices, which in turn are character
	// themselves
	// NOTE: this doesn't support anything except ASCII
	glBindBuffer(GL_ARRAY_BUFFER, text->chars);
	glBufferData(GL_ARRAY_BUFFER, text->len, str, GL_STATIC_DRAW);

#ifdef DEBUG
	GLenum gl_err = glGetError();
	if (gl_err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"failed to initialize text characters buffer "
			"(OpenGL error %d)",
			gl_err
		);
		return 0;
	}
#endif

	// setup the buffer of character coords
	GLfloat coords[text->len][2];
	text->width = text->height = 0;
	for (size_t c = 0; c < text->len; c++) {
		const struct Character *ch = font_get_char(
			text->font,
			str[c]
		);
		coords[c][0] = text->width;
		coords[c][1] = ch->bearing[1] - ch->size[1];
		text->width += ch->advance / 64.0;
		text->height = ch->size[1] > text->height ? ch->size[1] : text->height;
	}
	// align the Y coord so that all glyphs have negative coordinates
	for (size_t c = 0; c < text->len; c++) {
		coords[c][1] -= text->height - coords[c][1];
	}
	glBindBuffer(GL_ARRAY_BUFFER, text->coords);
	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(float) * text->len * 2,
		coords,
		GL_STATIC_DRAW
	);

#ifdef DEBUG
	gl_err = glGetError();
	if (gl_err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"failed to initialize text coords buffer "
			"(OpenGL error %d)",
			gl_err
		);
		return 0;
	}
#endif

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return 1;
}


int
text_render(struct Text *text)
{
	// skip drawing empty strings
	if (text->len == 0) {
		return 1;
	}

	// draw each character as instanced auto-generated primitive
	glBindVertexArray(text->vao);
	glDrawArraysInstanced(
		GL_TRIANGLE_STRIP,
		0,
		4,
		text->len
	);

#ifdef DEBUG
	GLenum gl_err = glGetError();
	if (gl_err != GL_NO_ERROR) {
		fprintf(
			stderr,
			"text primitive draw failed (OpenGL error %d)\n",
			gl_err
		);
		return 0;
	}
#endif

	glBindVertexArray(0);

	return 1;
}

void
text_destroy(struct Text *text)
{
	if (text) {
		glDeleteBuffers(1, &text->chars);
		glDeleteBuffers(1, &text->coords);
		glDeleteVertexArrays(1, &text->vao);
		free(text->string);
		free(text);
	}
}
