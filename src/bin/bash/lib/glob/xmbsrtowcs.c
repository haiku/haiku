/* xmbsrtowcs.c -- replacement function for mbsrtowcs */

/* Copyright (C) 2002-2004 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <bashansi.h>

/* <wchar.h>, <wctype.h> and <stdlib.h> are included in "shmbutil.h".
   If <wchar.h>, <wctype.h>, mbsrtowcs(), exist, HANDLE_MULTIBYTE
   is defined as 1. */
#include <shmbutil.h>

#if HANDLE_MULTIBYTE

#ifndef FREE
#  define FREE(x)	do { if (x) free (x); } while (0)
#endif
/* On some locales (ex. ja_JP.sjis), mbsrtowc doesn't convert 0x5c to U<0x5c>.
   So, this function is made for converting 0x5c to U<0x5c>. */

static mbstate_t local_state;
static int local_state_use = 0;

size_t
xmbsrtowcs (dest, src, len, pstate)
    wchar_t *dest;
    const char **src;
    size_t len;
    mbstate_t *pstate;
{
  mbstate_t *ps;
  size_t mblength, wclength, n;

  ps = pstate;
  if (pstate == NULL)
    {
      if (!local_state_use)
	{
	  memset (&local_state, '\0', sizeof(mbstate_t));
	  local_state_use = 1;
	}
      ps = &local_state;
    }

  n = strlen (*src);

  if (dest == NULL)
    {
      wchar_t *wsbuf;
      const char *mbs;
      mbstate_t psbuf;

      /* It doesn't matter if malloc fails here, since mbsrtowcs should do
	 the right thing with a NULL first argument. */
      wsbuf = (wchar_t *) malloc ((n + 1) * sizeof(wchar_t));
      mbs = *src;
      psbuf = *ps;

      wclength = mbsrtowcs (wsbuf, &mbs, n, &psbuf);

      if (wsbuf)
	free (wsbuf);
      return wclength;
    }
      
  for (wclength = 0; wclength < len; wclength++, dest++)
    {
      if (mbsinit(ps))
	{
	  if (**src == '\0')
	    {
	      *dest = L'\0';
	      *src = NULL;
	      return (wclength);
	    }
	  else if (**src == '\\')
	    {
	      *dest = L'\\';
	      mblength = 1;
	    }
	  else
	    mblength = mbrtowc(dest, *src, n, ps);
	}
      else
	mblength = mbrtowc(dest, *src, n, ps);

      /* Cannot convert multibyte character to wide character. */
      if (mblength == (size_t)-1 || mblength == (size_t)-2)
	return (size_t)-1;

      *src += mblength;
      n -= mblength;

      /* The multibyte string  has  been  completely  converted,
	 including  the terminating '\0'. */
      if (*dest == L'\0')
	{
	  *src = NULL;
	  break;
	}
    }

    return (wclength);
}

/* Convert a multibyte string to a wide character string. Memory for the
   new wide character string is obtained with malloc.

   The return value is the length of the wide character string. Returns a
   pointer to the wide character string in DESTP. If INDICESP is not NULL,
   INDICESP stores the pointer to the pointer array. Each pointer is to
   the first byte of each multibyte character. Memory for the pointer array
   is obtained with malloc, too.
   If conversion is failed, the return value is (size_t)-1 and the values
   of DESTP and INDICESP are NULL. */

#define WSBUF_INC 32

size_t
xdupmbstowcs (destp, indicesp, src)
    wchar_t **destp;	/* Store the pointer to the wide character string */
    char ***indicesp;	/* Store the pointer to the pointer array. */
    const char *src;	/* Multibyte character string */
{
  const char *p;	/* Conversion start position of src */
  wchar_t wc;		/* Created wide character by conversion */
  wchar_t *wsbuf;	/* Buffer for wide characters. */
  char **indices; 	/* Buffer for indices. */
  size_t wsbuf_size;	/* Size of WSBUF */
  size_t wcnum;		/* Number of wide characters in WSBUF */
  mbstate_t state;	/* Conversion State */

  /* In case SRC or DESP is NULL, conversion doesn't take place. */
  if (src == NULL || destp == NULL)
    {
      if (destp)
	*destp = NULL;
      return (size_t)-1;
    }

  memset (&state, '\0', sizeof(mbstate_t));
  wsbuf_size = WSBUF_INC;

  wsbuf = (wchar_t *) malloc (wsbuf_size * sizeof(wchar_t));
  if (wsbuf == NULL)
    {
      *destp = NULL;
      return (size_t)-1;
    }

  indices = NULL;
  if (indicesp)
    {
      indices = (char **) malloc (wsbuf_size * sizeof(char *));
      if (indices == NULL)
	{
	  free (wsbuf);
	  *destp = NULL;
	  return (size_t)-1;
	}
    }

  p = src;
  wcnum = 0;
  do
    {
      size_t mblength;	/* Byte length of one multibyte character. */

      if (mbsinit (&state))
	{
	  if (*p == '\0')
	    {
	      wc = L'\0';
	      mblength = 1;
	    }
	  else if (*p == '\\')
	    {
	      wc = L'\\';
	      mblength = 1;
	    }
	  else
	    mblength = mbrtowc(&wc, p, MB_LEN_MAX, &state);
	}
      else
	mblength = mbrtowc(&wc, p, MB_LEN_MAX, &state);

      /* Conversion failed. */
      if (MB_INVALIDCH (mblength))
	{
	  free (wsbuf);
	  FREE (indices);
	  *destp = NULL;
	  return (size_t)-1;
	}

      ++wcnum;

      /* Resize buffers when they are not large enough. */
      if (wsbuf_size < wcnum)
	{
	  wchar_t *wstmp;
	  char **idxtmp;

	  wsbuf_size += WSBUF_INC;

	  wstmp = (wchar_t *) realloc (wsbuf, wsbuf_size * sizeof (wchar_t));
	  if (wstmp == NULL)
	    {
	      free (wsbuf);
	      FREE (indices);
	      *destp = NULL;
	      return (size_t)-1;
	    }
	  wsbuf = wstmp;

	  if (indicesp)
	    {
	      idxtmp = (char **) realloc (indices, wsbuf_size * sizeof (char **));
	      if (idxtmp == NULL)
		{
		  free (wsbuf);
		  free (indices);
		  *destp = NULL;
		  return (size_t)-1;
		}
	      indices = idxtmp;
	    }
	}

      wsbuf[wcnum - 1] = wc;
      if (indices)
        indices[wcnum - 1] = (char *)p;
      p += mblength;
    }
  while (MB_NULLWCH (wc) == 0);

  /* Return the length of the wide character string, not including `\0'. */
  *destp = wsbuf;
  if (indicesp != NULL)
    *indicesp = indices;

  return (wcnum - 1);
}

#endif /* HANDLE_MULTIBYTE */
