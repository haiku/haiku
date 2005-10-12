/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


char *
strrchr(char const *s, int c)
{
	char const *last = c ? 0 : s;


	while (*s) {
		if (*s == c)
			last = s;

		s += 1;
	}

	return (char *)last;
}


char *
rindex(char const *s, int c)
{
	return strrchr(s, c);
}

