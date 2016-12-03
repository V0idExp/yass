#include "ioutils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

size_t
file_read(const char *filename, char **r_buf)
{
	assert(filename != NULL);
	assert(r_buf != NULL);

	*r_buf = NULL;

	size_t size = 0;

	FILE *fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "unable to open file '%s'\n", filename);
		goto error;
	}

	// read file size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	// allocate a buffer for its contents
	*r_buf = malloc(size + 1);
	if (!*r_buf) {
		fprintf(stderr, "could not allocate %lu bytes for file contents\n", size + 1);
		goto error;
	}

	(*r_buf)[size] = 0;  // NUL-terminator

	// read the file
	if (fread(*r_buf, 1, size, fp) != size) {
		fprintf(stderr, "I/O error\n");
		goto error;
	}

cleanup:
	if (fp) {
		fclose(fp);
	}
	return size;

error:
	size = 0;
	free(*r_buf);
	goto cleanup;
}
