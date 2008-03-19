/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <string.h>


int
strncmp(char const *a, char const *b, size_t count)
{
	while (count-- > 0) {
		int cmp = (unsigned char)*a - (unsigned char)*b++;
		if (cmp != 0 || *a++ == '\0')
			return cmp;
	}

	return 0;
}
