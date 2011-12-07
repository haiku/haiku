/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <errno.h>
#include <string.h>
#include <wchar.h>

#include <errno_private.h>
#include "LocaleBackend.h"


//#define TRACE_MBSRTOWCS
#ifdef TRACE_MBSRTOWCS
#	include <OS.h>
#	define TRACE(x) debug_printf x
#else
#	define TRACE(x) ;
#endif


using BPrivate::Libroot::gLocaleBackend;


extern "C" size_t
__mbsnrtowcs(wchar_t* dst, const char** src, size_t nmc, size_t len,
	mbstate_t* ps)
{
	TRACE(("mbsnrtowcs(%p, %p, %lu, %lu)\n", dst, *src, nmc, len));

	if (ps == NULL) {
		static mbstate_t internalMbState;
		ps = &internalMbState;
	}

	if (gLocaleBackend == NULL) {
		/*
		 * The POSIX locale is active. Since the POSIX locale only contains
		 * chars 0-127 and those ASCII chars are compatible with the UTF32
		 * values used in wint_t, we can just copy the bytes.
		 */
		size_t count = 0;
		if (dst == NULL) {
			// only count number of required wide characters
			for (const char* srcEnd = *src + nmc; *src < srcEnd;
					++*src, ++count) {
				if (*src < 0) {
					// char is non-ASCII
					__set_errno(EILSEQ);
					return (size_t)-1;
				}
				if (**src == 0) {
					memset(ps, 0, sizeof(mbstate_t));
					*src = NULL;
					break;
				}
			}
		} else {
			// "convert" the characters
			for (; count < len; ++*src, ++count) {
				if (*src < 0) {
					// char is non-ASCII
					__set_errno(EILSEQ);
					return (size_t)-1;
				}
				*dst++ = (wchar_t)*src;
				if (*src == 0) {
					memset(ps, 0, sizeof(mbstate_t));
					*src = NULL;
					break;
				}
			}
		}

		TRACE(("mbsnrtowcs returns %lx and src %p\n", count, *src));

		return count;
	}

	size_t result = 0;
	status_t status = gLocaleBackend->MultibyteStringToWchar(dst, len, src, nmc,
		ps, result);

	if (status == B_BAD_DATA) {
		TRACE(("mbsnrtowc(): setting errno to EILSEQ\n"));
		__set_errno(EILSEQ);
		result = (size_t)-1;
	} else if (status != B_OK) {
		TRACE(("mbsnrtowc(): setting errno to EINVAL (status: %lx)\n", status));
		__set_errno(EINVAL);
		result = (size_t)-1;
	}

	TRACE(("mbsnrtowcs returns %lx and src %p\n", result, *src));

	return result;
}


extern "C"
B_DEFINE_WEAK_ALIAS(__mbsnrtowcs, mbsnrtowcs);


extern "C" size_t
__mbsrtowcs(wchar_t* dst, const char** src, size_t len, mbstate_t* ps)
{
	if (ps == NULL) {
		static mbstate_t internalMbState;
		ps = &internalMbState;
	}

	size_t srcLen = gLocaleBackend == NULL ? strlen(*src) : (size_t)-1;

	return __mbsnrtowcs(dst, src, srcLen, len, ps);
}


extern "C"
B_DEFINE_WEAK_ALIAS(__mbsrtowcs, mbsrtowcs);
