#pragma once

#include <stdio.h>

enum {
	ERR_NO_MEM,
	ERR_SDL,
	ERR_OPENGL,
	ERR_LIBPNG,
	ERR_FILE_READ,
	ERR_FILE_BAD,
	ERR_SCRIPT_INIT,
	ERR_SCRIPT_LOAD,
	ERR_SCRIPT_CALL,
	ERR_INVALID_ANCHORS,
	ERR_MAX
};

#define error(code) error_push(code, __LINE__, __FILE__, __func__)

void
error_push(int code, unsigned long line, const char *file, const char *where);

int
error_is_set(void);

void
error_dump(FILE *file);

void
error_clear(void);