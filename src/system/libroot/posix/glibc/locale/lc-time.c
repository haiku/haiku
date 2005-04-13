/* Define current locale data for LC_TIME category.
   Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.
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

#include <bits/libc-lock.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include "localeinfo.h"

_NL_CURRENT_DEFINE (LC_TIME);

/* Some of the functions here must not be used while setlocale is called.  */
__libc_lock_define (extern, __libc_setlocale_lock)


static int era_initialized;

static struct era_entry *eras;
static size_t num_eras;
static int alt_digits_initialized;
static const char **alt_digits;


static int walt_digits_initialized;
static const wchar_t **walt_digits;


void
_nl_postload_time (void)
{
  /* Prepare lazy initialization of `era' and `alt_digits' array.  */
  era_initialized = 0;
  alt_digits_initialized = 0;
  walt_digits_initialized = 0;
}

#define ERA_DATE_CMP(a, b) \
  (a[0] < b[0] || (a[0] == b[0] && (a[1] < b[1]				      \
				    || (a[1] == b[1] && a[2] <= b[2]))))

static void
_nl_init_era_entries (void)
{
  size_t cnt;

  __libc_lock_lock (__libc_setlocale_lock);

  if (era_initialized == 0)
    {
      size_t new_num_eras = _NL_CURRENT_WORD (LC_TIME,
					      _NL_TIME_ERA_NUM_ENTRIES);
      if (new_num_eras == 0)
	{
	  free (eras);
	  eras = NULL;
	}
      else
	{
	  if (num_eras != new_num_eras)
	    eras = (struct era_entry *) realloc (eras,
						 new_num_eras
						 * sizeof (struct era_entry));
	  if (eras == NULL)
	    {
	      num_eras = 0;
	      eras = NULL;
	    }
          else
	    {
	      const char *ptr = _NL_CURRENT (LC_TIME, _NL_TIME_ERA_ENTRIES);
	      num_eras = new_num_eras;

	      for (cnt = 0; cnt < num_eras; ++cnt)
		{
		  const char *base_ptr = ptr;
		  memcpy ((void *) (eras + cnt), (const void *) ptr,
			  sizeof (uint32_t) * 8);

		  if (ERA_DATE_CMP(eras[cnt].start_date,
				   eras[cnt].stop_date))
		    if (eras[cnt].direction == (uint32_t) '+')
		      eras[cnt].absolute_direction = 1;
		    else
		      eras[cnt].absolute_direction = -1;
		  else
		    if (eras[cnt].direction == (uint32_t) '+')
		      eras[cnt].absolute_direction = -1;
		    else
		      eras[cnt].absolute_direction = 1;

		  /* Skip numeric values.  */
		  ptr += sizeof (uint32_t) * 8;

		  /* Set and skip era name.  */
		  eras[cnt].era_name = ptr;
		  ptr = strchr (ptr, '\0') + 1;

		  /* Set and skip era format.  */
		  eras[cnt].era_format = ptr;
		  ptr = strchr (ptr, '\0') + 1;

		  ptr += 3 - (((ptr - (const char *) base_ptr) + 3) & 3);

		  /* Set and skip wide era name.  */
		  eras[cnt].era_wname = (wchar_t *) ptr;
		  ptr = (char *) (wcschr ((wchar_t *) ptr, L'\0') + 1);

		  /* Set and skip wide era format.  */
		  eras[cnt].era_wformat = (wchar_t *) ptr;
		  ptr = (char *) (wcschr ((wchar_t *) ptr, L'\0') + 1);
		}
	    }
	}

      era_initialized = 1;
    }

  __libc_lock_unlock (__libc_setlocale_lock);
}


struct era_entry *
_nl_get_era_entry (const struct tm *tp)
{
  struct era_entry *result;
  int32_t tdate[3];
  size_t cnt;

  tdate[0] = tp->tm_year;
  tdate[1] = tp->tm_mon;
  tdate[2] = tp->tm_mday;

  if (era_initialized == 0)
    _nl_init_era_entries ();

  /* Now compare date with the available eras.  */
  for (cnt = 0; cnt < num_eras; ++cnt)
    if ((ERA_DATE_CMP(eras[cnt].start_date, tdate)
	 && ERA_DATE_CMP(tdate, eras[cnt].stop_date))
	|| (ERA_DATE_CMP(eras[cnt].stop_date, tdate)
	    && ERA_DATE_CMP(tdate, eras[cnt].start_date)))
      break;

  result = cnt < num_eras ? &eras[cnt] : NULL;

  return result;
}


struct era_entry *
_nl_select_era_entry (int cnt)
{
  if (era_initialized == 0)
    _nl_init_era_entries ();

  return &eras[cnt];
}


const char *
_nl_get_alt_digit (unsigned int number)
{
  const char *result;

  __libc_lock_lock (__libc_setlocale_lock);

  if (alt_digits_initialized == 0)
    {
      alt_digits_initialized = 1;

      if (alt_digits == NULL)
	alt_digits = malloc (100 * sizeof (const char *));

      if (alt_digits != NULL)
	{
	  const char *ptr = _NL_CURRENT (LC_TIME, ALT_DIGITS);
	  size_t cnt;

	  if (alt_digits != NULL)
	    for (cnt = 0; cnt < 100; ++cnt)
	      {
		alt_digits[cnt] = ptr;

		/* Skip digit format. */
		ptr = strchr (ptr, '\0') + 1;
	      }
	}
    }

  result = alt_digits != NULL && number < 100 ? alt_digits[number] : NULL;

  __libc_lock_unlock (__libc_setlocale_lock);

  return result;
}


const wchar_t *
_nl_get_walt_digit (unsigned int number)
{
  const wchar_t *result;

  __libc_lock_lock (__libc_setlocale_lock);

  if (walt_digits_initialized == 0)
    {
      walt_digits_initialized = 1;

      if (walt_digits == NULL)
	walt_digits = malloc (100 * sizeof (const uint32_t *));

      if (walt_digits != NULL)
	{
	  const wchar_t *ptr = _NL_CURRENT_WSTR (LC_TIME, _NL_WALT_DIGITS);
	  size_t cnt;

	  for (cnt = 0; cnt < 100; ++cnt)
	    {
	      walt_digits[cnt] = ptr;

	      /* Skip digit format. */
	      ptr = wcschr (ptr, L'\0') + 1;
	    }
	}
    }

  result = walt_digits != NULL && number < 100 ? walt_digits[number] : NULL;

  __libc_lock_unlock (__libc_setlocale_lock);

  return (wchar_t *) result;
}


int
_nl_parse_alt_digit (const char **strp)
{
  const char *str = *strp;
  int result = -1;
  size_t cnt;
  size_t maxlen = 0;

  __libc_lock_lock (__libc_setlocale_lock);

  if (alt_digits_initialized == 0)
    {
      alt_digits_initialized = 1;

      if (alt_digits == NULL)
	alt_digits = malloc (100 * sizeof (const char *));

      if (alt_digits != NULL)
	{
	  const char *ptr = _NL_CURRENT (LC_TIME, ALT_DIGITS);

	  if (alt_digits != NULL)
	    for (cnt = 0; cnt < 100; ++cnt)
	      {
		alt_digits[cnt] = ptr;

		/* Skip digit format. */
		ptr = strchr (ptr, '\0') + 1;
	      }
	}
    }

  /* Matching is not unambiguos.  The alternative digits could be like
     I, II, III, ... and the first one is a substring of the second
     and third.  Therefore we must keep on searching until we found
     the longest possible match.  Note that this is not specified in
     the standard.  */
  for (cnt = 0; cnt < 100; ++cnt)
    {
      size_t len = strlen (alt_digits[cnt]);

      if (len > maxlen && strncmp (alt_digits[cnt], str, len) == 0)
	{
	  maxlen = len;
	  result = (int) cnt;
	}
    }

  __libc_lock_unlock (__libc_setlocale_lock);

  if (result != -1)
    *strp += maxlen;

  return result;
}


static void
free_mem (void)
{
  free (alt_digits);
  free (walt_digits);
}
text_set_element (__libc_subfreeres, free_mem);
