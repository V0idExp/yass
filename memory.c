#include "memory.h"
#include <stdlib.h>
#include <string.h>

void*
alloc0(size_t size)
{
	void *bytes = malloc(size);
	if (!size) {
		return NULL;
	}
	memset(bytes, 0, size);
	return bytes;
}