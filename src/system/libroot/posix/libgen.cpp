/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <libgen.h>
#include <string.h>


char*
basename(char *filepath)
{
	if (filepath == NULL || filepath[0] == '\0')
		return (char *)".";

	size_t length = strlen(filepath);
	/* Remove trailing slashes if any */
	while (filepath[--length] == '/' && length)
		filepath[length] = '\0';

	char *last = strrchr(filepath, '/');
	/* If no slash were found return the whole string */
	if (last == NULL)
		return filepath;

	/* If the next char is the end it means we got only "/" 
	 * and we don't have to truncate */
	if (*(last + 1) != '\0')
		++last;

	return last;
}


char*
dirname(char *filepath)
{
	if (filepath == NULL || filepath[0] == '\0')
		return (char *)".";

	size_t length = strlen(filepath);
	/* Remove trailing slashes if any */
	while (filepath[--length] == '/' && length)
		filepath[length] = '\0';

	char *last = strrchr(filepath, '/');
	/* If no slash were found return a dot */
	if (last == NULL)
		return (char *)".";

	/* In case we got just "/" don't truncate it */
	if (last == filepath)
		last++;

	*last = '\0';

	return filepath;
}
