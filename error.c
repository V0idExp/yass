#include "error.h"
#include <stdlib.h>

#define MAX_ERRORS 64

static struct Error {
	int code;
	unsigned long line;
	const char *file;
	const char *where;
} errors[MAX_ERRORS];

static unsigned error_count = 0;

static const char *err_msgs[] = {
	// ERR_NO_MEM
	"out of memory",
	// ERR_SDL
	"SDL internal error",
	// ERR_OPENGL
	"OpenGL internal error",
	// ERR_FILE_READ
	"file read error",
	// ERR_SCRIPT_INIT
	"script environment initialization failure",
	// ERR_SCRIPT_LOAD
	"script file load failure",
	// ERR_SCRIPT_CALL
	"script function call failure",
};

void
error_push(int code, unsigned long line, const char *file, const char *where)
{
	if (error_count == MAX_ERRORS) {
		fprintf(stderr, "maximum number of errors reached\n");
		abort();
	}
	struct Error *e = &errors[error_count++];
	e->code = code;
	e->line = line;
	e->file = file;
	e->where = where;
}

int
error_is_set(void)
{
	return error_count != 0;
}

void
error_dump(FILE *file)
{
	if (error_count) {
		for (unsigned i = 0; i < error_count; i++) {
			struct Error *e = &errors[i];
			const char *msg = (
				e->code < ERR_MAX ?
				err_msgs[e->code] :
				"unknown error"
			);
			int ok = fprintf(
				file,
				"%s:%lu (%s): %s\n",
				e->file,
				e->line,
				e->where,
				msg
			);
			if (!ok) {
				fprintf(
					stderr,
					"failed to dump error traceback\n"
				);
				abort();
			}
		}
	}
}

void
error_clear(void)
{
	error_count = 0;
}