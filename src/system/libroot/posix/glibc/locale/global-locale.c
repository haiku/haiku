/* Locale object representing the global locale controlled by setlocale.
   Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <locale.h>
#include "localeinfo.h"

#define DEFINE_CATEGORY(category, category_name, items, a) \
extern struct locale_data _nl_C_##category; weak_extern (_nl_C_##category)
#include "categories.def"
#undef	DEFINE_CATEGORY

/* Defined in locale/C-ctype.c.  */
extern const char _nl_C_LC_CTYPE_class[] attribute_hidden;
extern const char _nl_C_LC_CTYPE_toupper[] attribute_hidden;
extern const char _nl_C_LC_CTYPE_tolower[] attribute_hidden;
weak_extern (_nl_C_LC_CTYPE_class)
weak_extern (_nl_C_LC_CTYPE_toupper)
weak_extern (_nl_C_LC_CTYPE_tolower)

/* Here we define the locale object maintained by setlocale.
   The references in the initializer are weak, so the parts of
   the structure that are never referred to will be zero.  */

struct __locale_struct _nl_global_locale attribute_hidden =
  {
    .__locales =
    {
#define DEFINE_CATEGORY(category, category_name, items, a) \
      [category] = &_nl_C_##category,
#include "categories.def"
#undef	DEFINE_CATEGORY
    },
    .__names =
    {
      [LC_ALL] = _nl_C_name,
#define DEFINE_CATEGORY(category, category_name, items, a) \
      [category] = _nl_C_name,
#include "categories.def"
#undef	DEFINE_CATEGORY
    },
    .__ctype_b = (const unsigned short int *) _nl_C_LC_CTYPE_class + 128,
    .__ctype_tolower = (const int *) _nl_C_LC_CTYPE_tolower + 128,
    .__ctype_toupper = (const int *) _nl_C_LC_CTYPE_toupper + 128
  };

#include <tls.h>
#if USE_TLS && HAVE___THREAD
/* The tsd macros don't permit an initializer.  */
__thread void *__libc_tsd_LOCALE = &_nl_global_locale;
#else
__libc_tsd_define (, LOCALE)
/* This is a bad kludge presuming the variable name used by the macros.
   Using typeof makes sure to barf if we do not match the macro definition.
   This ifndef is a further bad kludge for Hurd, where there is an explicit
   initialization.  */
# ifndef _HURD_THREADVAR_H
__typeof (__libc_tsd_LOCALE_data) __libc_tsd_LOCALE_data = &_nl_global_locale;
# endif
#endif
