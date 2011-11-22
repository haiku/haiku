/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <stdint.h>

#include <wchar_private.h>


int
__wctob(wint_t c)
{
	char internalBuffer[MB_LEN_MAX];

	int32_t byteCount = wcrtomb(internalBuffer, c, NULL);
	if (byteCount != 1)
		return EOF;

	return (int)(unsigned char)internalBuffer[0];
}


B_DEFINE_WEAK_ALIAS(__wctob, wctob);
