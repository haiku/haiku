/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <string.h>


int
memcmp(const void *_a, const void *_b, size_t count)
{
	const unsigned char *a = (const unsigned char *)_a;
	const unsigned char *b = (const unsigned char *)_b;

	while (count-- > 0) {
		int cmp = *a++ - *b++;
		if (cmp != 0)
			return cmp;
	}

	return 0;
}
