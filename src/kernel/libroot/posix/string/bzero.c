/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>

void
bzero(void *dst, size_t count)
{
	memset(dst, 0, count);
}

