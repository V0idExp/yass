#pragma once

struct Text {
	struct Font *font;
	size_t len;
	GLuint vao;
	GLuint coords;
	GLuint chars;
	unsigned width;
	unsigned height;
	char *string;
};

struct Text*
text_new(struct Font *font);

int
text_set_string(struct Text *text, const char *str);

int
text_render(struct Text *text);

void
text_destroy(struct Text *text);
