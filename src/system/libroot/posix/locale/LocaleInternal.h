/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>


#define LOCALE_T_MAGIC 'LOCA'


namespace BPrivate {
namespace Libroot {


status_t GetLocalesFromEnvironment(int category, const char** locales);


// The pointer in the TLS will point to this struct.
struct ThreadLocale {
	struct LocaleBackendData* threadLocaleInfo;

	const unsigned short int* ctype_b;
	const int* ctype_tolower;
	const int* ctype_toupper;
	const unsigned short int* mb_cur_max;
};

ThreadLocale* GetCurrentThreadLocale();


}	// namespace Libroot
}	// namespace BPrivate
