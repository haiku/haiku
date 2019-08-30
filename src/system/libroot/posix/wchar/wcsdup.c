/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <stdlib.h>
#include <string.h>

#include <errno_private.h>
#include <wchar_private.h>


wchar_t*
__wcsdup(const wchar_t* wcs)
{
	if (wcs == NULL)
		return NULL;

	{
		size_t bufferSize = (wcslen(wcs) + 1) * sizeof(wchar_t);
		wchar_t* dest = malloc(bufferSize);
		if (dest == NULL) {
			__set_errno(ENOMEM);
			return NULL;
		}

		memcpy(dest, wcs, bufferSize);

		return dest;
	}
}


B_DEFINE_WEAK_ALIAS(__wcsdup, wcsdup);
