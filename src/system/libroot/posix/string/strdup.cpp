/* 
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>
#include <stdlib.h>


char*
strdup(const char *string)
{
	char* copied;
	size_t length;

	// unlike the standard strdup() function, the BeOS implementation
	// handles NULL strings gracefully
	if (string == NULL)
		return NULL;

	length = strlen(string) + 1;

	if ((copied = (char *)malloc(length)) == NULL)
		return NULL;

	memcpy(copied, string, length);
	return copied;
}
