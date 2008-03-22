/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <sys/types.h>
#include <string.h>

#ifdef bcopy
#	undef bcopy
#endif

void *bcopy(void const *src, void *dest, size_t count);

void *
bcopy(void const *src, void *dest, size_t count)
{
	return memmove(dest, src, count);
}

