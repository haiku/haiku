/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <sys/types.h>
#include <string.h>


static char *___strtok = NULL;


char *
strtok_r(char *s, char const *ct, char **save_ptr)
{
	char *sbegin, *send;
	
	if (!s && !save_ptr)
		return NULL;

	sbegin  = s ? s : *save_ptr;
	if (!sbegin)
		return NULL;

	sbegin += strspn(sbegin, ct);
	if (*sbegin == '\0') {
		if (save_ptr)
			*save_ptr = NULL;
		return NULL;
	}

	send = strpbrk(sbegin, ct);
	if (send && *send != '\0')
		*send++ = '\0';
	if (save_ptr)
		*save_ptr = send;

	return sbegin;
}


char *
strtok(char *s, char const *ct)
{
	return strtok_r(s, ct, &___strtok);
}

