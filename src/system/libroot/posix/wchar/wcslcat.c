/*
** Copyright 2002, Manuel J. Petit.
** Copyright 2011, Oliver Tappe <zooey@hirschkafer.de>.
** All rights reserved. Distributed under the terms of the NewOS License.
*/

#include <wchar_private.h>


/** Concatenates the source string to the destination, writes
 *	as much as "maxLength" bytes to the dest string.
 *	Always null terminates the string as long as maxLength is
 *	larger than the dest string.
 *	Returns the length of the string that it tried to create
 *	to be able to easily detect string truncation.
 */

size_t
__wcslcat(wchar_t* dest, const wchar_t* source, size_t maxLength)
{
	size_t destLength = __wcsnlen(dest, maxLength);
	size_t i;

	if (destLength == maxLength) {
		// the destination is already full
		return destLength + __wcslen(source);
	}

	dest += destLength;
	maxLength -= destLength;

	for (i = 0; i < maxLength - 1 && *source != L'\0'; ++i)
		*dest++ = *source++;

	*dest = '\0';

	return destLength + i + __wcslen(source);
}


B_DEFINE_WEAK_ALIAS(__wcslcat, wcslcat);
