/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _THREAD_LOCALE_H
#define _THREAD_LOCALE_H


#include "LocaleBackend.h"


namespace BPrivate {
namespace Libroot {


// The pointer in the TLS will point to this struct.
struct ThreadLocale {
	LocaleBackendData*  threadLocaleInfo;

	const unsigned short int* ctype_b;
	const int* ctype_tolower;
	const int* ctype_toupper;
};


ThreadLocale* GetCurrentThreadLocale();


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _THREAD_LOCALE_H
