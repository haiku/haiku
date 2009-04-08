/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>


char*
strsep(char** string, const char* delimiters)
{
	if (*string == NULL)
		return NULL;

	// find the end of the token
	char* token = *string;
	char* end = token;
	while (*end != '\0' && strchr(delimiters, *end) == NULL)
		end++;

	// terminate the token and update the string pointer
	if (*end != '\0') {
		*end = '\0';
		*string = end + 1;
	} else
		*string = NULL;

	return token;
}
