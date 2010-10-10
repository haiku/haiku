/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <wchar.h>


int
wcswidth(const wchar_t* wcstring, size_t n)
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
