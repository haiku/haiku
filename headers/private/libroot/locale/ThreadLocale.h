/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef _THREAD_LOCALE_H
#define _THREAD_LOCALE_H


#include "LocaleBackend.h"


namespace BPrivate {
namespace Libroot {


// This struct is taken from glibc's __locale_struct
// from xlocale.h.
// It will also be used by glibc so it should have the same
// layout.
struct GlibcLocaleStruct {
    void *__locales[7]; /* 7 = __LC_LAST. */

    const unsigned short int *__ctype_b;
    const int *__ctype_tolower;
    const int *__ctype_toupper;
};


// Taken from glibc's bits/locale.h
// glibc uses different codes from our the native locale.h.
// This should be the values used for the indexes
// in GlibcLocaleStruct.
enum {
    GLIBC_LC_CTYPE = 0,
    GLIBC_LC_NUMERIC = 1,
    GLIBC_LC_TIME = 2,
    GLIBC_LC_COLLATE = 3,
    GLIBC_LC_MONETARY = 4,
    GLIBC_LC_MESSAGES = 5,
    GLIBC_LC_ALL = 6,
};


// The pointer in the TLS will point to this struct.
struct ThreadLocale {
    GlibcLocaleStruct   glibcLocaleStruct;
    LocaleBackendData*  threadLocaleInfo;
};


ThreadLocale* GetCurrentThreadLocale();


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _THREAD_LOCALE_H
