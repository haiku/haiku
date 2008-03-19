/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <stdbool.h>
#include <string.h>


int
strcmp(char const *a, char const *b)
{
	while (true) {
		int cmp = (unsigned char)*a - (unsigned char)*b++;
		if (cmp != 0 || *a++ == '\0')
			return cmp;
	}
}
