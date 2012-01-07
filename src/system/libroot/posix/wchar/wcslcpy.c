/*
** Copyright 2002, Manuel J. Petit.
** Copyright 2011, Oliver Tappe <zooey@hirschkafer.de>.
** All rights reserved. Distributed under the terms of the NewOS License.
*/

#include <wchar_private.h>


size_t
__wcslcpy(wchar_t* dest, const wchar_t* source, size_t maxLength)
{
	size_t i;

	if (maxLength == 0)
		return __wcslen(source);

	for (i = 0; i < maxLength - 1 && *source != L'\0'; ++i)
		*dest++ = *source++;

	*dest++ = 0;

	return i + __wcslen(source);
}


B_DEFINE_WEAK_ALIAS(__wcslcpy, wcslcpy);
