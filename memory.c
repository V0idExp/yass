#include "error.h"
#include "memory.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void*
alloc0(size_t size)
{
	void *bytes = malloc(size);
	if (!size) {
		error(ERR_NO_MEM);
		return NULL;
	}
	memset(bytes, 0, size);
	return bytes;
}

void*
copy(const void *ptr, size_t size)
{
	assert(ptr != NULL);
	void *clone = malloc(size);
	if (!clone) {
		error(ERR_NO_MEM);
		return NULL;
	}
	memcpy(clone, ptr, size);
	return clone;
}

void
destroy(void *data)
{
	free(data);
}