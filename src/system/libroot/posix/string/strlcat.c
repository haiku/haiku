/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>


/** Concatenates the source string to the destination, writes
 *	as much as "maxLength" bytes to the dest string.
 *	Always null terminates the string as long as maxLength is
 *	larger than the dest string.
 *	Returns the length of the string that it tried to create
 *	to be able to easily detect string truncation.
 */

size_t
strlcat(char *dest, const char *source, size_t maxLength)
{
	size_t destLength = strnlen(dest, maxLength), i;

	// This returns the wrong size, but it's all we can do
	if (maxLength == destLength)
		return destLength + strlen(source);

	dest += destLength;
	maxLength -= destLength;

	for (i = 0; i < maxLength - 1 && source[i]; i++) {
		dest[i] = source[i];
	}

	dest[i] = '\0';

	return destLength + i + strlen(source + i);
}

