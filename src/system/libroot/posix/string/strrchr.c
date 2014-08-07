/*
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <string.h>
#include <strings.h>
#include <sys/types.h>


char *
strrchr(char const *s, int c)
{
	char const *last = NULL;

	for (;; s++) {
		if (*s == (char)c)
			last = s;
		if (*s == '\0')
			break;
	}

	return (char *)last;
}


char *
rindex(char const *s, int c)
{
	return strrchr(s, c);
}

