/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


char *
strcat(char *dest,  char const*src)
{
	char *tmp = dest;

	while(*dest)
		dest++;
	while((*dest++ = *src++) != '\0')
		;

	return tmp;
}

