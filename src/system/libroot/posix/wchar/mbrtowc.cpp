/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <errno.h>
#include <string.h>
#include <wchar.h>

#include <errno_private.h>
#include "LocaleBackend.h"


using BPrivate::Libroot::gLocaleBackend;


extern "C" size_t
__mbrtowc(wchar_t* pwc, const char* s, size_t n, mbstate_t* ps)
{
	if (ps == NULL) {
		static mbstate_t internalMbState;
		ps = &internalMbState;
	}

	if (s == NULL)
		return __mbrtowc(NULL, "", 1, ps);

	if (gLocaleBackend == NULL) {
		if (*s == '\0') {
			memset(ps, 0, sizeof(mbstate_t));

			if (pwc != NULL)
				*pwc = 0;

			return 0;
		}

		/*
		 * The POSIX locale is active. Since the POSIX locale only contains
		 * chars 0-127 and those ASCII chars are compatible with the UTF32
		 * values used in wint_t, we can just return the byte.
		 */

		if (*s < 0) {
			// char is non-ASCII
			__set_errno(EILSEQ);
			return (size_t)-1;
		}

		if (pwc != NULL)
			*pwc = *s;

		return 1;
	}

	size_t lengthUsed;
	status_t status
		= gLocaleBackend->MultibyteToWchar(pwc, s, n, ps, lengthUsed);

	if (status == B_BAD_INDEX)
		return (size_t)-2;

	if (status == B_BAD_DATA) {
		__set_errno(EILSEQ);
		return (size_t)-1;
	}

	if (status != B_OK) {
		__set_errno(EINVAL);
		return (size_t)-1;
	}

	return lengthUsed;
}


extern "C"
B_DEFINE_WEAK_ALIAS(__mbrtowc, mbrtowc);
