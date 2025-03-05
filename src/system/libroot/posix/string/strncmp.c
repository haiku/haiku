/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <stdint.h>
#include <string.h>


#define LACKS_ZERO_BYTE(value) \
	(((value - 0x01010101) & ~value & 0x80808080) == 0)

int
strncmp(char const *a, char const *b, size_t count)
{
	if ((((addr_t)a) & 3) == 0 && (((addr_t)b) & 3) == 0) {
		uint32_t *a32 = (uint32_t *)a;
		uint32_t *b32 = (uint32_t *)b;

		while (count >= 4 && *a32 == *b32 && LACKS_ZERO_BYTE((*a32))) {
			a32++;
			b32++;
			count -= 4;
		}
		a = (const char *)a32;
		b = (const char *)b32;
	}

	while (count-- > 0) {
		int cmp = (unsigned char)*a - (unsigned char)*b++;
		if (cmp != 0 || *a++ == '\0')
			return cmp;
	}

	return 0;
}
