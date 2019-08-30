/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <sys/types.h>
#include <string.h>


#ifdef bcmp
#	undef bcmp
#endif

int bcmp(void const *a, const void *b, size_t bytes);

int
bcmp(void const *a, const void *b, size_t bytes)
{
	return memcmp(a, b, bytes);
}

