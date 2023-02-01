/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ctype.h>

#include <tls.h>
#include <kernel/OS.h>
#include <ThreadLocale.h>


// From glibc's localeinfo.h
extern BPrivate::Libroot::GlibcLocaleStruct _nl_global_locale;


namespace BPrivate {
namespace Libroot {


static void DestroyThreadLocale(void* ptr)
{
	ThreadLocale* threadLocale = (ThreadLocale*)ptr;
	delete threadLocale;
}


ThreadLocale*
GetCurrentThreadLocale()
{
	ThreadLocale* threadLocale = (ThreadLocale*)tls_get(TLS_LOCALE_SLOT);
	if (threadLocale == NULL) {
		threadLocale = new ThreadLocale();
		threadLocale->glibcLocaleStruct = _nl_global_locale;
		threadLocale->threadLocaleInfo = NULL;
		on_exit_thread(DestroyThreadLocale, threadLocale);
		tls_set(TLS_LOCALE_SLOT, threadLocale);
	}
	return threadLocale;
}


// Exported so that glibc could also use.
extern "C" GlibcLocaleStruct*
_nl_current_locale()
{
	return &GetCurrentThreadLocale()->glibcLocaleStruct;
}


extern "C" const unsigned short**
__ctype_b_loc()
{
	return &GetCurrentThreadLocale()->glibcLocaleStruct.__ctype_b;
}


extern "C" const int**
__ctype_tolower_loc()
{
	return &GetCurrentThreadLocale()->glibcLocaleStruct.__ctype_tolower;
}


extern "C" const int**
__ctype_toupper_loc()
{
	return &GetCurrentThreadLocale()->glibcLocaleStruct.__ctype_toupper;
}


}	// namespace Libroot
}	// namespace BPrivate
