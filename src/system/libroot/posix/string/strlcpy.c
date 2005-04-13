/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


size_t
strlcpy(char *dst, char const *src, size_t s)
{
	size_t i= 0;

	if (!s)
		return strlen(src);

	for (i = 0; ((i < s - 1) && src[i]); i++) {
		dst[i] = src[i];
	}

	dst[i] = 0;

	return i + strlen(src + i);
}
