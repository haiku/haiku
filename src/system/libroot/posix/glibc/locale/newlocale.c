/* Return a reference to locale information record.
   Copyright (C) 1996, 1997, 1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#include <argz.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>

#include "localeinfo.h"


/* Constant data defined in setlocale.c.  */
extern struct locale_data *const _nl_C[];

/* Use this when we come along an error.  */
#define ERROR_RETURN							      \
  do {									      \
    __set_errno (EINVAL);						      \
    return NULL;							      \
  } while (0)


__locale_t
__newlocale (int category_mask, const char *locale, __locale_t base)
{
  /* Intermediate memory for result.  */
  const char *newnames[__LC_LAST];
  struct __locale_struct result;
  __locale_t result_ptr;
  char *locale_path;
  size_t locale_path_len;
  const char *locpath_var;
  int cnt;

  /* We treat LC_ALL in the same way as if all bits were set.  */
  if (category_mask == 1 << LC_ALL)
    category_mask = (1 << __LC_LAST) - 1 - (1 << LC_ALL);

  /* Sanity check for CATEGORY argument.  */
  if ((category_mask & ~((1 << __LC_LAST) - 1 - (1 << LC_ALL))) != 0)
    ERROR_RETURN;

  /* `newlocale' does not support asking for the locale name. */
  if (locale == NULL)
    ERROR_RETURN;

  /* Allocate memory for the result.  */
  if (base != NULL)
    result = *base;
  else
    {
      /* Fill with pointers to C locale data.  */
      for (cnt = 0; cnt < __LC_LAST; ++cnt)
	if (cnt != LC_ALL)
	  result.__locales[cnt] = _nl_C[cnt];
    }

  /* If no category is to be set we return BASE if available or a
     dataset using the C locale data.  */
  if (category_mask == 0)
    {
      result_ptr = (__locale_t) malloc (sizeof (struct __locale_struct));
      if (result_ptr == NULL)
	return NULL;
      *result_ptr = result;

      goto update;
    }

  /* We perhaps really have to load some data.  So we determine the
     path in which to look for the data now.  The environment variable
     `LOCPATH' must only be used when the binary has no SUID or SGID
     bit set.  */
  locale_path = NULL;
  locale_path_len = 0;

  locpath_var = getenv ("LOCPATH");
  if (locpath_var != NULL && locpath_var[0] != '\0')
    if (__argz_create_sep (locpath_var, ':',
			   &locale_path, &locale_path_len) != 0)
      return NULL;

  if (__argz_append (&locale_path, &locale_path_len,
		     LOCALEDIR, sizeof (LOCALEDIR)) != 0)
    return NULL;

  /* Get the names for the locales we are interested in.  We either
     allow a composite name or a single name.  */
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (cnt != LC_ALL)
      newnames[cnt] = locale;
  if (strchr (locale, ';') != NULL)
    {
      /* This is a composite name.  Make a copy and split it up.  */
      char *np = strdupa (locale);
      char *cp;

      while ((cp = strchr (np, '=')) != NULL)
	{
	  for (cnt = 0; cnt < __LC_LAST; ++cnt)
	    if (cnt != LC_ALL
		&& (size_t) (cp - np) == _nl_category_name_sizes[cnt]
		&& memcmp (np, _nl_category_names[cnt], cp - np) == 0)
	      break;

	  if (cnt == __LC_LAST)
	    /* Bogus category name.  */
	    ERROR_RETURN;

	  /* Found the category this clause sets.  */
	  newnames[cnt] = ++cp;
	  cp = strchr (cp, ';');
	  if (cp != NULL)
	    {
	      /* Examine the next clause.  */
	      *cp = '\0';
	      np = cp + 1;
	    }
	  else
	    /* This was the last clause.  We are done.  */
	    break;
	}

      for (cnt = 0; cnt < __LC_LAST; ++cnt)
	if (cnt != LC_ALL
	    && (category_mask & 1 << cnt) != 0 && newnames[cnt] == locale)
	  /* The composite name did not specify the category we need.  */
	  ERROR_RETURN;
    }

  /* Now process all categories we are interested in.  */
  for (cnt = 0; cnt < __LC_LAST; ++cnt)
    if (cnt != LC_ALL && (category_mask & 1 << cnt) != 0)
      {
	result.__locales[cnt] = _nl_find_locale (locale_path, locale_path_len,
						 cnt, &newnames[cnt]);
	if (result.__locales[cnt] == NULL)
	  return NULL;
      }

  /* We successfully loaded all required data.  */
  if (base == NULL)
    {
      /* Allocate new structure.  */
      result_ptr = (__locale_t) malloc (sizeof (struct __locale_struct));
      if (result_ptr == NULL)
	return NULL;

    }
  else
    /* We modify the base structure.  */
    result_ptr = base;

  *result_ptr = result;

  /* Update the special members.  */
 update:
  {
    union locale_data_value *ctypes = result_ptr->__locales[LC_CTYPE]->values;
    result_ptr->__ctype_b = (const unsigned short int *)
      ctypes[_NL_ITEM_INDEX (_NL_CTYPE_CLASS)].string + 128;
    result_ptr->__ctype_tolower = (const int *)
      ctypes[_NL_ITEM_INDEX (_NL_CTYPE_TOLOWER)].string + 128;
    result_ptr->__ctype_toupper = (const int *)
      ctypes[_NL_ITEM_INDEX (_NL_CTYPE_TOUPPER)].string + 128;
  }

  return result_ptr;
}
