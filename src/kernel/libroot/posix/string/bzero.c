/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <sys/types.h>
#include <string.h>


#ifdef bzero
#	undef bzero
#endif

void bzero(void *dest, size_t count);

void
bzero(void *dest, size_t count)
{
	memset(dest, 0, count);
}

