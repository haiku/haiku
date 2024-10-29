/*
 * Copyright 2024, Jérôme Duval, jerome.duval@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>

#include <errno.h>
#include <stdint.h>


extern "C" void*
reallocarray(void* ptr, size_t nelem, size_t elsize)
{
	if (elsize != 0 && nelem != 0 && nelem > SIZE_MAX / elsize) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(ptr, nelem * elsize);
}
