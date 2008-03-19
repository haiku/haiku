/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <stdio.h>
#include <string.h>


int
main(int argc, char** argv)
{
	char a[] = { -26, '\0' };
	char b[] = { '.', '\0' };

	printf("strcmp(): %d\n", strcmp(a, b));
	printf("memcmp(): %d\n", memcmp(a, b, 1));
	printf("strncmp(): %d\n", strncmp(a, b, 1));
	printf("strcasecmp(): %d\n", strcasecmp(a, b));
	printf("strncasecmp(): %d\n", strncasecmp(a, b, 1));

	return 0;
}
