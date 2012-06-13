/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
*/

// TODO: Replace these with optimized implementations.


#include <string.h>


void *
memcpy(void *dest, const void *src, size_t count)
{
	const unsigned char *s = src;
	unsigned char *d = dest;

	for (; count != 0; count--) {
		*d++ = *s++;
	}

	return dest;
}


void *
memset(void *dest, int val, size_t count)
{
	unsigned char *d = dest;

	for (; count != 0; count--) {
		*d++ = static_cast<unsigned char>(val);
	}

	return dest;
}
