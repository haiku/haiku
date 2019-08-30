/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


wint_t
__btowc(int c)
{
	static mbstate_t internalMbState;
	char character = (char)c;
	wchar_t wc;

	if (c == EOF)
		return WEOF;

	if (c == '\0')
		return L'\0';

	{
		int byteCount = __mbrtowc(&wc, &character, 1, &internalMbState);

		if (byteCount != 1)
			return WEOF;
	}

	return wc;
}


B_DEFINE_WEAK_ALIAS(__btowc, btowc);
