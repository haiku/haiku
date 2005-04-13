/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


void *
memchr(void const *buf, int c, size_t len)
{
	unsigned char const *b = buf;
	unsigned char x = (c&0xff);
	size_t i;

	for (i = 0; i < len; i++) {
		if (b[i] == x)
			return (void*)(b + i);
	}

	return NULL;
}

