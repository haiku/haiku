/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


size_t
strlen(char const *s)
{
	size_t i = 0;

	while (s[i]) {
		i += 1;
	}

	return i;
}
