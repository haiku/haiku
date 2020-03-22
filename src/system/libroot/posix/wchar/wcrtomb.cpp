/*
** Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <errno.h>
#include <string.h>
#include <wchar.h>

#include <errno_private.h>
#include <LocaleBackend.h>


//#define TRACE_WCRTOMB
#ifdef TRACE_WCRTOMB
#	include <OS.h>
#	define TRACE(x) debug_printf x
#else
#	define TRACE(x) ;
#endif


using BPrivate::Libroot::gLocaleBackend;


extern "C" size_t
__wcrtomb(char* s, wchar_t wc, mbstate_t* ps)
{
	if (ps == NULL) {
		static mbstate_t internalMbState;
		ps = &internalMbState;
	}

	if (s == NULL)
		wc = 0;

	if (gLocaleBackend == NULL) {
		/*
		 * The POSIX locale is active. Since the POSIX locale only contains
		 * chars 0-127 and those ASCII chars are compatible with the UTF32
		 * values used in wint_t, we can just return the byte.
		 */

		if (wc > 127) {
			// char is non-ASCII
			__set_errno(EILSEQ);
			return (size_t)-1;
		}

		if (s != NULL)
			*s = char(wc);

		return 1;
	}

	size_t lengthUsed;
	status_t status = gLocaleBackend->WcharToMultibyte(s, wc, ps, lengthUsed);

	if (status == B_BAD_INDEX)
		return (size_t)-2;

	if (status == B_BAD_DATA) {
		TRACE(("mbrtowc(): setting errno to EILSEQ\n"));
		__set_errno(EILSEQ);
		return (size_t)-1;
	}

	if (status != B_OK) {
		TRACE(("wcrtomb(): setting errno to EINVAL (status: %lx)\n", status));
		__set_errno(EINVAL);
		return (size_t)-1;
	}

	return lengthUsed;
}


B_DEFINE_WEAK_ALIAS(__wcrtomb, wcrtomb);
