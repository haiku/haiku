/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOCALE_IMPL_H
#define _LOCALE_IMPL_H

#include <locale.h>

#define __nl_langinfo_l nl_langinfo_l

locale_t __current_locale_t();
locale_t __posix_locale_t();
#define CURRENT_LOCALE (__current_locale_t())
#define C_LOCALE (__posix_locale_t())

#endif
