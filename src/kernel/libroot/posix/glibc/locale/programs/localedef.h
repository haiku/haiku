/* General definitions for localedef(1).
   Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

#ifndef _LOCALEDEF_H
#define _LOCALEDEF_H	1

/* Get the basic locale definitions.  */
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>

#include "repertoire.h"
#include "../locarchive.h"


/* We need a bitmask for the locales.  */
enum
{
  CTYPE_LOCALE = 1 << LC_CTYPE,
  NUMERIC_LOCALE = 1 << LC_NUMERIC,
  TIME_LOCALE = 1 << LC_TIME,
  COLLATE_LOCALE = 1 << LC_COLLATE,
  MONETARY_LOCALE = 1 << LC_MONETARY,
  MESSAGES_LOCALE = 1 << LC_MESSAGES,
  PAPER_LOCALE = 1 << LC_PAPER,
  NAME_LOCALE = 1 << LC_NAME,
  ADDRESS_LOCALE = 1 << LC_ADDRESS,
  TELEPHONE_LOCALE = 1 << LC_TELEPHONE,
  MEASUREMENT_LOCALE = 1 << LC_MEASUREMENT,
  IDENTIFICATION_LOCALE = 1 << LC_IDENTIFICATION,
  ALL_LOCALES = (1 << LC_CTYPE
		 | 1 << LC_NUMERIC
		 | 1 << LC_TIME
		 | 1 << LC_COLLATE
		 | 1 << LC_MONETARY
		 | 1 << LC_MESSAGES
		 | 1 << LC_PAPER
		 | 1 << LC_NAME
		 | 1 << LC_ADDRESS
		 | 1 << LC_TELEPHONE
		 | 1 << LC_MEASUREMENT
		 | 1 << LC_IDENTIFICATION)
};


/* Opaque types for the different locales.  */
struct locale_ctype_t;
struct locale_collate_t;
struct locale_monetary_t;
struct locale_numeric_t;
struct locale_time_t;
struct locale_messages_t;
struct locale_paper_t;
struct locale_name_t;
struct locale_address_t;
struct locale_telephone_t;
struct locale_measurement_t;
struct locale_identification_t;


/* Definitions for the locale.  */
struct localedef_t
{
  struct localedef_t *next;

  const char *name;

  int needed;
  int avail;

  union
  {
    void *generic;
    struct locale_ctype_t *ctype;
    struct locale_collate_t *collate;
    struct locale_monetary_t *monetary;
    struct locale_numeric_t *numeric;
    struct locale_time_t *time;
    struct locale_messages_t *messages;
    struct locale_paper_t *paper;
    struct locale_name_t *name;
    struct locale_address_t *address;
    struct locale_telephone_t *telephone;
    struct locale_measurement_t *measurement;
    struct locale_identification_t *identification;
  } categories[__LC_LAST];

  size_t len[__LC_LAST];

  const char *copy_name[__LC_LAST];

  const char *repertoire_name;
};


/* Global variables of the localedef program.  */
extern int verbose;
extern int be_quiet;
extern int oldstyle_tables;
extern const char *repertoire_global;
extern int max_locarchive_open_retry;
extern bool no_archive;
extern const char *alias_file;


/* Prototypes for a few program-wide used functions.  */
extern void *xmalloc (size_t __n);
extern void *xcalloc (size_t __n, size_t __size);
extern void *xrealloc (void *__p, size_t __n);
extern char *xstrdup (const char *__str);


/* Wrapper to switch LC_CTYPE back to the locale specified in the
   environment for output.  */
#define WITH_CUR_LOCALE(stmt)					\
  do {								\
      int saved_errno = errno;					\
      const char *cur_locale_ = setlocale (LC_CTYPE, NULL);	\
      setlocale (LC_CTYPE, "");					\
      errno = saved_errno; 					\
      stmt;							\
      setlocale (LC_CTYPE, cur_locale_);			\
  } while (0)


/* Mark given locale as to be read.  */
extern struct localedef_t *add_to_readlist (int locale, const char *name,
					    const char *repertoire_name,
					    int generate,
					    struct localedef_t *copy_locale);

/* Find the information for the locale NAME.  */
extern struct localedef_t *find_locale (int locale, const char *name,
					const char *repertoire_name,
					const struct charmap_t *charmap);

/* Load (if necessary) the information for the locale NAME.  */
extern struct localedef_t *load_locale (int locale, const char *name,
					const char *repertoire_name,
					const struct charmap_t *charmap,
					struct localedef_t *copy_locale);


/* Open the locale archive.  */
extern void open_archive (struct locarhandle *ah, bool readonly);

/* Close the locale archive.  */
extern void close_archive (struct locarhandle *ah);

/* Add given locale data to the archive.  */
extern int add_locale_to_archive (struct locarhandle *ah, const char *name,
				  locale_data_t data, bool replace);

/* Add content of named directories to locale archive.  */
extern int add_locales_to_archive (size_t nlist, char *list[], bool replace);

/* Removed named locales from archive.  */
extern int delete_locales_from_archive (size_t nlist, char *list[]);

/* List content of locale archive.  */
extern void show_archive_content (int verbose);

#endif /* localedef.h */
