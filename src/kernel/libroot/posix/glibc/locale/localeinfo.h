/* Declarations for internal libc locale interfaces
   Copyright (C) 1995-2001, 2002 Free Software Foundation, Inc.
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
#include <locale.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>

#include <intl/loadinfo.h>	/* For loaded_l10nfile definition.  */

/* Magic number at the beginning of a locale data file for CATEGORY.  */
#define	LIMAGIC(category)	((unsigned int) (0x20000828 ^ (category)))

/* Two special weight constants for the collation data.  */
#define IGNORE_CHAR	2

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
  enum				/* Flavor of storage used for those.  */
  {
    ld_malloced,		/* Both are malloc'd.  */
    ld_mapped,			/* name is malloc'd, filedata mmap'd */
    ld_archive			/* Both point into mmap'd archive regions.  */
  } alloc;

  /* This provides a slot for category-specific code to cache data computed
     about this locale.  That code can set a cleanup function to deallocate
     the data.  */
  struct
  {
    void (*cleanup) (struct locale_data *) internal_function;
    union
    {
      void *data;
      struct lc_time_data *time;
      const struct gconv_fcts *ctype;
    } u;
  } private;

  unsigned int usage_count;	/* Counter for users.  */

  int use_translit;		/* Nonzero if the mb*towv*() and wc*tomb()
				   functions should use transliteration.  */

  unsigned int nstrings;	/* Number of strings below.  */
  union locale_data_value
  {
    const uint32_t *wstr;
    const char *string;
    unsigned int word;		/* Note endian issues vs 64-bit pointers.  */
  }
  values __flexarr;	/* Items, usually pointers into `filedata'.  */
};

/* We know three kinds of collation sorting rules.  */
enum coll_sort_rule
{
  illegal_0__,
  sort_forward,
  sort_backward,
  illegal_3__,
  sort_position,
  sort_forward_position,
  sort_backward_position,
  sort_mask
};

/* We can map the types of the entries into a few categories.  */
enum value_type
{
  none,
  string,
  stringarray,
  byte,
  bytearray,
  word,
  stringlist,
  wordarray,
  wstring,
  wstringarray,
  wstringlist
};


/* Definitions for `era' information from LC_TIME.  */
#define ERA_NAME_FORMAT_MEMBERS 4
#define ERA_M_NAME   0
#define ERA_M_FORMAT 1
#define ERA_W_NAME   2
#define ERA_W_FORMAT 3


/* Structure to access `era' information from LC_TIME.  */
struct era_entry
{
  uint32_t direction;		/* Contains '+' or '-'.  */
  int32_t offset;
  int32_t start_date[3];
  int32_t stop_date[3];
  const char *era_name;
  const char *era_format;
  const wchar_t *era_wname;
  const wchar_t *era_wformat;
  int absolute_direction;
  /* absolute direction:
     +1 indicates that year number is higher in the future. (like A.D.)
     -1 indicates that year number is higher in the past. (like B.C.)  */
};

/* Structure caching computed data about information from LC_TIME.
   The `private.time' member of `struct locale_data' points to this.  */
struct lc_time_data
{
  struct era_entry *eras;
  size_t num_eras;
  int era_initialized;

  const char **alt_digits;
  const wchar_t **walt_digits;
  int alt_digits_initialized;
  int walt_digits_initialized;
};


/* LC_CTYPE specific:
   Hardwired indices for standard wide character translation mappings.  */
enum
{
  __TOW_toupper = 0,
  __TOW_tolower = 1
};


/* LC_CTYPE specific:
   Access a wide character class with a single character index.
   _ISCTYPE (c, desc) = iswctype (btowc (c), desc).
   c must be an `unsigned char'.  desc must be a nonzero wctype_t.  */
#define _ISCTYPE(c, desc) \
  (((((const uint32_t *) (desc)) - 8)[(c) >> 5] >> ((c) & 0x1f)) & 1)

extern const char *const _nl_category_names[__LC_LAST] attribute_hidden;
extern const size_t _nl_category_name_sizes[__LC_LAST] attribute_hidden;

/* Name of the standard locales.  */
extern const char _nl_C_name[] attribute_hidden;
extern const char _nl_POSIX_name[] attribute_hidden;

/* The standard codeset.  */
extern const char _nl_C_codeset[] attribute_hidden;

/* This is the internal locale_t object that holds the global locale
   controlled by calls to setlocale.  A thread's TSD locale pointer
   points to this when `uselocale (LC_GLOBAL_LOCALE)' is in effect.  */
extern struct __locale_struct _nl_global_locale attribute_hidden;

/* This fetches the thread-local locale_t pointer, either one set with
   uselocale or &_nl_global_locale.  */
#define _NL_CURRENT_LOCALE	((__locale_t) __libc_tsd_get (LOCALE))
#include <bits/libc-tsd.h>
__libc_tsd_define (extern, LOCALE)


/* For static linking it is desireable to avoid always linking in the code
   and data for every category when we can tell at link time that they are
   unused.  We can manage this playing some tricks with weak references.
   But with thread-local locale settings, it becomes quite ungainly unless
   we can use __thread variables.  So only in that case do we attempt this.  */
#if !defined SHARED && defined HAVE___THREAD && defined HAVE_WEAK_SYMBOLS
# include <tls.h>
# if USE_TLS
#  define NL_CURRENT_INDIRECT	1
# endif
#endif

#ifdef NL_CURRENT_INDIRECT

/* For each category declare the thread-local variable for the current
   locale data.  This has an extra indirection so it points at the
   __locales[CATEGORY] element in either _nl_global_locale or the current
   locale object set by uselocale, which points at the actual data.  The
   reason for having these variables is so that references to particular
   categories will link in the lc-CATEGORY.c module to define this symbol,
   and we arrange that linking that module is what brings in all the code
   associated with this category.  */
#define DEFINE_CATEGORY(category, category_name, items, a) \
extern __thread struct locale_data *const *_nl_current_##category \
  attribute_hidden attribute_tls_model_ie;
#include "categories.def"
#undef	DEFINE_CATEGORY

/* Return a pointer to the current `struct locale_data' for CATEGORY.  */
#define _NL_CURRENT_DATA(category)	(*_nl_current_##category)

/* Extract the current CATEGORY locale's string for ITEM.  */
#define _NL_CURRENT(category, item) \
  ((*_nl_current_##category)->values[_NL_ITEM_INDEX (item)].string)

/* Extract the current CATEGORY locale's string for ITEM.  */
#define _NL_CURRENT_WSTR(category, item) \
  ((wchar_t *) (*_nl_current_##category)->values[_NL_ITEM_INDEX (item)].wstr)

/* Extract the current CATEGORY locale's word for ITEM.  */
#define _NL_CURRENT_WORD(category, item) \
  ((uint32_t) (*_nl_current_##category)->values[_NL_ITEM_INDEX (item)].word)

/* This is used in lc-CATEGORY.c to define _nl_current_CATEGORY.  */
#define _NL_CURRENT_DEFINE(category) \
  __thread struct locale_data *const *_nl_current_##category \
    attribute_hidden = &_nl_global_locale.__locales[category]; \
  asm (_NL_CURRENT_DEFINE_STRINGIFY (ASM_GLOBAL_DIRECTIVE) \
       " " __SYMBOL_PREFIX "_nl_current_" #category "_used\n" \
       _NL_CURRENT_DEFINE_ABS (_nl_current_##category##_used, 1));
#define _NL_CURRENT_DEFINE_STRINGIFY(x) _NL_CURRENT_DEFINE_STRINGIFY_1 (x)
#define _NL_CURRENT_DEFINE_STRINGIFY_1(x) #x
#ifdef HAVE_ASM_SET_DIRECTIVE
# define _NL_CURRENT_DEFINE_ABS(sym, val) ".set " #sym ", " #val
#else
# define _NL_CURRENT_DEFINE_ABS(sym, val) #sym " = " #val
#endif

#else

/* All categories are always loaded in the shared library, so there is no
   point in having lots of separate symbols for linking.  */

/* Return a pointer to the current `struct locale_data' for CATEGORY.  */
# define _NL_CURRENT_DATA(category) \
  (_NL_CURRENT_LOCALE->__locales[category])

/* Extract the current CATEGORY locale's string for ITEM.  */
# define _NL_CURRENT(category, item) \
  (_NL_CURRENT_DATA (category)->values[_NL_ITEM_INDEX (item)].string)

/* Extract the current CATEGORY locale's string for ITEM.  */
# define _NL_CURRENT_WSTR(category, item) \
  ((wchar_t *) _NL_CURRENT_DATA (category)->values[_NL_ITEM_INDEX (item)].wstr)

/* Extract the current CATEGORY locale's word for ITEM.  */
# define _NL_CURRENT_WORD(category, item) \
  ((uint32_t) _NL_CURRENT_DATA (category)->values[_NL_ITEM_INDEX (item)].word)

/* This is used in lc-CATEGORY.c to define _nl_current_CATEGORY.  */
# define _NL_CURRENT_DEFINE(category) \
  /* No per-category variable here. */

#endif


/* Default search path if no LOCPATH environment variable.  */
extern const char _nl_default_locale_path[] attribute_hidden;

/* Load the locale data for CATEGORY from the file specified by *NAME.
   If *NAME is "", use environment variables as specified by POSIX, and
   fill in *NAME with the actual name used.  If LOCALE_PATH is not null,
   those directories are searched for the locale files.  If it's null,
   the locale archive is checked first and then _nl_default_locale_path
   is searched for locale files.  */
extern struct locale_data *_nl_find_locale (const char *locale_path,
					    size_t locale_path_len,
					    int category, const char **name)
     internal_function attribute_hidden;

/* Try to load the file described by FILE.  */
extern void _nl_load_locale (struct loaded_l10nfile *file, int category)
     internal_function attribute_hidden;

/* Free all resource.  */
extern void _nl_unload_locale (struct locale_data *locale)
     internal_function attribute_hidden;

/* Free the locale and give back all memory if the usage count is one.  */
extern void _nl_remove_locale (int locale, struct locale_data *data)
     internal_function attribute_hidden;

/* Find the locale *NAMEP in the locale archive, and return the
   internalized data structure for its CATEGORY data.  If this locale has
   already been loaded from the archive, just returns the existing data
   structure.  If successful, sets *NAMEP to point directly into the mapped
   archive string table; that way, the next call can short-circuit strcmp.  */
extern struct locale_data *_nl_load_locale_from_archive (int category,
							 const char **namep)
     internal_function attribute_hidden;

/* Subroutine of setlocale's __libc_subfreeres hook.  */
extern void _nl_archive_subfreeres (void) attribute_hidden;

/* Validate the contents of a locale file and set up the in-core
   data structure to point into the data.  This leaves the `alloc'
   and `name' fields uninitialized, for the caller to fill in.
   If any bogons are detected in the data, this will refuse to
   intern it, and return a null pointer instead.  */
extern struct locale_data *_nl_intern_locale_data (int category,
						   const void *data,
						   size_t datasize)
     internal_function attribute_hidden;


/* Return `era' entry which corresponds to TP.  Used in strftime.  */
extern struct era_entry *_nl_get_era_entry (const struct tm *tp,
					    struct locale_data *lc_time)
     internal_function attribute_hidden;

/* Return `era' cnt'th entry .  Used in strptime.  */
extern struct era_entry *_nl_select_era_entry (int cnt,
					       struct locale_data *lc_time)
          internal_function attribute_hidden;

/* Return `alt_digit' which corresponds to NUMBER.  Used in strftime.  */
extern const char *_nl_get_alt_digit (unsigned int number,
				      struct locale_data *lc_time)
          internal_function attribute_hidden;

/* Similar, but now for wide characters.  */
extern const wchar_t *_nl_get_walt_digit (unsigned int number,
					  struct locale_data *lc_time)
     internal_function attribute_hidden;

/* Parse string as alternative digit and return numeric value.  */
extern int _nl_parse_alt_digit (const char **strp,
				struct locale_data *lc_time)
     internal_function attribute_hidden;

/* Postload processing.  */
extern void _nl_postload_ctype (void);

/* Functions used for the `private.cleanup' hook.  */
extern void _nl_cleanup_time (struct locale_data *)
     internal_function attribute_hidden;


#endif	/* localeinfo.h */
