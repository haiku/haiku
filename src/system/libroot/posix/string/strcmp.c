/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <stdint.h>
#include <string.h>


#define LACKS_ZERO_BYTE(value) \
	(((value - 0x01010101) & ~value & 0x80808080) == 0)

int
strcmp(char const *a, char const *b)
{
	if ((((addr_t)a) & 3) == 0 && (((addr_t)b) & 3) == 0) {
		uint32_t *a32 = (uint32_t *)a;
		uint32_t *b32 = (uint32_t *)b;

		while (*a32 == *b32 && LACKS_ZERO_BYTE((*a32))) {
			a32++;
			b32++;
		}
		a = (const char *)a32;
		b = (const char *)b32;
	}

	while (*a == *b && *a != 0) {
		a++;
		b++;
	}

	return (unsigned char)*a - (unsigned char)*b;
}
