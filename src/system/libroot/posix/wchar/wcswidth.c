/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <wchar_private.h>


int
__wcswidth(const wchar_t* wcstring, size_t n)
{
	int width = 0;

	while (*wcstring && n-- > 0) {
		int w = wcwidth(*wcstring++);
		if (w < 0)
			return -1;

		width += w;
	}

	return width;
}


B_DEFINE_WEAK_ALIAS(__wcswidth, wcswidth);
