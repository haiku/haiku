/* xmbsrtowcs.c -- replacement function for mbsrtowcs */

/* Copyright (C) 2002 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */
#include <config.h>

#include <bashansi.h>

/* <wchar.h>, <wctype.h> and <stdlib.h> are included in "shmbutil.h".
   If <wchar.h>, <wctype.h>, mbsrtowcs(), exist, HANDLE_MULTIBYTE
   is defined as 1. */
#include <shmbutil.h>

#if HANDLE_MULTIBYTE
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

  n = strlen(*src) + 1;

  if (dest == NULL)
    {
      wchar_t *wsbuf;
      char *mbsbuf, *mbsbuf_top;
      mbstate_t psbuf;

      wsbuf = (wchar_t *) malloc ((n + 1) * sizeof(wchar_t));
      mbsbuf_top = mbsbuf = (char *) malloc (n + 1);
      memcpy(mbsbuf, *src, n + 1);
      psbuf = *ps;

      wclength = mbsrtowcs (wsbuf, (const char **)&mbsbuf, n, &psbuf);

      free (wsbuf);
      free (mbsbuf_top);
      return wclength;
    }
      
  for(wclength = 0; wclength < len; wclength++, dest++)
    {
      if(mbsinit(ps))
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
#endif /* HANDLE_MULTIBYTE */
