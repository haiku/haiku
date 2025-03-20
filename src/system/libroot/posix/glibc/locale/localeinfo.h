/* Declarations for internal libc locale interfaces
   Copyright (C) 1995, 96, 97, 98, 99,2000,2001 Free Software Foundation, Inc.
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

#ifndef _LOCALEINFO_H
#define _LOCALEINFO_H 1

#include <stddef.h>
#include <langinfo.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>

// __locale_struct
#include <xlocale.h>
// LC_* values
#include <locale.h>

/* This has to be changed whenever a new locale is defined.  */
#define __LC_LAST	7

/* We use a special value for the usage counter in `locale_data' to
   signal that this data must never be removed anymore.  */
#define MAX_USAGE_COUNT (UINT_MAX - 1)
#define UNDELETABLE	UINT_MAX

/* Structure describing locale data in core for a category.  */
struct locale_data
{
  const char *name;
  const char *filedata;		/* Region mapping the file data.  */
  off_t filesize;		/* Size of the file (and the region).  */
  int mmaped;			/* If nonzero the data is mmaped.  */

  unsigned int usage_count;	/* Counter for users.  */

  int use_translit;		/* Nonzero if the mb*towv*() and wc*tomb()
				   functions should use transliteration.  */
  const char *options;		/* Extra options from the locale name,
				   not used in the path to the locale data.  */

  unsigned int nstrings;	/* Number of strings below.  */
  union locale_data_value
  {
    const uint32_t *wstr;
    const char *string;
    unsigned int word;
  }
  values __flexarr;	/* Items, usually pointers into `filedata'.  */
};


/* Name of the standard locales.  */
extern const char _nl_C_name[];
extern const char _nl_POSIX_name[];

/* The standard codeset.  */
extern const char _nl_C_codeset[];

/* This is the internal locale_t object that holds the global locale
   controlled by calls to setlocale.  A thread's TSD locale pointer
   points to this when `uselocale (LC_GLOBAL_LOCALE)' is in effect.  */
extern struct __locale_struct _nl_global_locale;

extern struct __locale_struct* _nl_current_locale();
#define _NL_CURRENT_LOCALE        (_nl_current_locale())

/* Return a pointer to the current `struct __locale_data' for CATEGORY.  */
#define _NL_CURRENT_DATA(category) \
  (_NL_CURRENT_LOCALE->__locales[category])

/* Extract the current CATEGORY locale's string for ITEM.  */
#define _NL_CURRENT(category, item) \
  (_NL_CURRENT_DATA (category)->values[_NL_ITEM_INDEX (item)].string)

/* Extract the current CATEGORY locale's string for ITEM.  */
#define _NL_CURRENT_WSTR(category, item) \
  ((wchar_t *) _NL_CURRENT_DATA (category)->values[_NL_ITEM_INDEX (item)].wstr)

/* Extract the current CATEGORY locale's word for ITEM.  */
#define _NL_CURRENT_WORD(category, item) \
  ((uint32_t) _NL_CURRENT_DATA (category)->values[_NL_ITEM_INDEX (item)].word)

/* This is used in lc-CATEGORY.c to define _nl_current_CATEGORY.  */
#define _NL_CURRENT_DEFINE(category) \
  /* No per-category variable here. */

/* Postload processing.  */
extern void _nl_postload_ctype (void);
extern void _nl_postload_time (void);


#endif	/* localeinfo.h */
