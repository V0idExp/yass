#pragma once

#include <stddef.h>

/**
 * Allocates given amount of bytes and zeroes it.
 */
void*
alloc0(size_t size);

/**
 * Allocates given type and zeroes it.
 */
#define make(t) (alloc0(sizeof(t)))