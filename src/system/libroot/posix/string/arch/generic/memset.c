/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


void *
memset(void *s, int c, size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

#ifdef __ARM__
void* __aeabi_memset(void *s, int c, size_t count)
	__attribute__((__alias__("memset")));
#endif
