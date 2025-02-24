/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <stdint.h>
#include <string.h>


int
memcmp(const void *_a, const void *_b, size_t count)
{
	const unsigned char *a = (const unsigned char *)_a;
	const unsigned char *b = (const unsigned char *)_b;

	if ((((addr_t)a) & 3) == 0 && (((addr_t)b) & 3) == 0) {
		uint32_t *a32 = (uint32_t *)a;
		uint32_t *b32 = (uint32_t *)b;

		while (count >= 4 && *a32 == *b32) {
			a32++;
			b32++;
			count -= 4;
		}
		a = (const unsigned char *)a32;
		b = (const unsigned char *)b32;
	}

	while (count-- > 0) {
		int cmp = *a++ - *b++;
		if (cmp != 0)
			return cmp;
	}

	return 0;
}
