/* Copyright (C) 1995-1999,2000,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Ulrich Drepper <drepper@cygnus.com>, 1995.

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

#include <assert.h>
#include <langinfo.h>
#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#ifndef STRING_TYPE
# define STRING_TYPE char
# define USTRING_TYPE unsigned char
# ifdef USE_IN_EXTENDED_LOCALE_MODEL
#  define STRXFRM __strxfrm_l
# else
#  define STRXFRM strxfrm
# endif
# define STRCMP strcmp
# define STRLEN strlen
# define STPNCPY __stpncpy
# define WEIGHT_H "../locale/weight.h"
# define SUFFIX	MB
# define L(arg) arg
#endif

#define CONCAT(a,b) CONCAT1(a,b)
#define CONCAT1(a,b) a##b

#include "../locale/localeinfo.h"


#ifndef WIDE_CHAR_VERSION

/* We need UTF-8 encoding of numbers.  */
static inline int
utf8_encode (char *buf, int val)
{
  int retval;

  if (val < 0x80)
    {
      *buf++ = (char) val;
      retval = 1;
    }
  else
    {
      int step;

      for (step = 2; step < 6; ++step)
	if ((val & (~(uint32_t)0 << (5 * step + 1))) == 0)
	  break;
      retval = step;

      *buf = (unsigned char) (~0xff >> step);
      --step;
      do
	{
	  buf[step] = 0x80 | (val & 0x3f);
	  val >>= 6;
	}
      while (--step > 0);
      *buf |= val;
    }

  return retval;
}
#endif


#ifndef USE_IN_EXTENDED_LOCALE_MODEL
size_t
STRXFRM (STRING_TYPE *dest, const STRING_TYPE *src, size_t n)
#else
size_t
STRXFRM (STRING_TYPE *dest, const STRING_TYPE *src, size_t n, __locale_t l)
#endif
{
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
  struct locale_data *current = l->__locales[LC_COLLATE];
  uint_fast32_t nrules = current->values[_NL_ITEM_INDEX (_NL_COLLATE_NRULES)].word;
#else
  uint32_t nrules = _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);
#endif
  /* We don't assign the following values right away since it might be
     unnecessary in case there are no rules.  */
  const unsigned char *rulesets;
  const int32_t *table;
  const USTRING_TYPE *weights;
  const USTRING_TYPE *extra;
  const int32_t *indirect;
  uint_fast32_t pass;
  size_t needed;
  const USTRING_TYPE *usrc;
  size_t srclen = STRLEN (src);
  int32_t *idxarr;
  unsigned char *rulearr;
  size_t idxmax;
  size_t idxcnt;
  int use_malloc;

#include WEIGHT_H

  if (nrules == 0)
    {
      if (n != 0)
	STPNCPY (dest, src, MIN (srclen + 1, n));

      return srclen;
    }

#ifdef USE_IN_EXTENDED_LOCALE_MODEL
  rulesets = (const unsigned char *)
    current->values[_NL_ITEM_INDEX (_NL_COLLATE_RULESETS)].string;
  table = (const int32_t *)
    current->values[_NL_ITEM_INDEX (CONCAT(_NL_COLLATE_TABLE,SUFFIX))].string;
  weights = (const USTRING_TYPE *)
    current->values[_NL_ITEM_INDEX (CONCAT(_NL_COLLATE_WEIGHT,SUFFIX))].string;
  extra = (const USTRING_TYPE *)
    current->values[_NL_ITEM_INDEX (CONCAT(_NL_COLLATE_EXTRA,SUFFIX))].string;
  indirect = (const int32_t *)
    current->values[_NL_ITEM_INDEX (CONCAT(_NL_COLLATE_INDIRECT,SUFFIX))].string;
#else
  rulesets = (const unsigned char *)
    _NL_CURRENT (LC_COLLATE, _NL_COLLATE_RULESETS);
  table = (const int32_t *)
    _NL_CURRENT (LC_COLLATE, CONCAT(_NL_COLLATE_TABLE,SUFFIX));
  weights = (const USTRING_TYPE *)
    _NL_CURRENT (LC_COLLATE, CONCAT(_NL_COLLATE_WEIGHT,SUFFIX));
  extra = (const USTRING_TYPE *)
    _NL_CURRENT (LC_COLLATE, CONCAT(_NL_COLLATE_EXTRA,SUFFIX));
  indirect = (const int32_t *)
    _NL_CURRENT (LC_COLLATE, CONCAT(_NL_COLLATE_INDIRECT,SUFFIX));
#endif
  use_malloc = 0;

  assert (((uintptr_t) table) % __alignof__ (table[0]) == 0);
  assert (((uintptr_t) weights) % __alignof__ (weights[0]) == 0);
  assert (((uintptr_t) extra) % __alignof__ (extra[0]) == 0);
  assert (((uintptr_t) indirect) % __alignof__ (indirect[0]) == 0);

  /* Handle an empty string as a special case.  */
  if (srclen == 0)
    {
      if (n != 0)
        *dest = L('\0');
      return 0;
    }

  /* We need the elements of the string as unsigned values since they
     are used as indeces.  */
  usrc = (const USTRING_TYPE *) src;

  /* Perform the first pass over the string and while doing this find
     and store the weights for each character.  Since we want this to
     be as fast as possible we are using `alloca' to store the temporary
     values.  But since there is no limit on the length of the string
     we have to use `malloc' if the string is too long.  We should be
     very conservative here.  */
  if (! __libc_use_alloca (srclen))
    {
      idxarr = (int32_t *) malloc ((srclen + 1) * (sizeof (int32_t) + 1));
      rulearr = (unsigned char *) &idxarr[srclen];

      if (idxarr == NULL)
	/* No memory.  Well, go with the stack then.

	   XXX Once this implementation is stable we will handle this
	   differently.  Instead of precomputing the indeces we will
	   do this in time.  This means, though, that this happens for
	   every pass again.  */
	goto try_stack;
      use_malloc = 1;
    }
  else
    {
    try_stack:
      idxarr = (int32_t *) alloca (srclen * sizeof (int32_t));
      rulearr = (unsigned char *) alloca (srclen + 1);
    }
  /* This element is only read, the value never used but to determine
     another value which then is ignored.  */
  rulearr[srclen] = '\0';

  idxmax = 0;
  do
    {
      int32_t tmp = findidx (&usrc);
      rulearr[idxmax] = tmp >> 24;
      idxarr[idxmax] = tmp & 0xffffff;

      ++idxmax;
    }
  while (*usrc != L('\0'));

  /* Now the passes over the weights.  We now use the indeces we found
     before.  */
  needed = 0;
  for (pass = 0; pass < nrules; ++pass)
    {
      size_t backw_stop = ~0ul;
      int rule = rulesets[rulearr[0] * nrules + pass];
      /* We assume that if a rule has defined `position' in one section
	 this is true for all of them.  */
      int position = rule & sort_position;

      if (position == 0)
	{
	  for (idxcnt = 0; idxcnt < idxmax; ++idxcnt)
	    {
	      if ((rule & sort_forward) != 0)
		{
		  size_t len;

		  if (backw_stop != ~0ul)
		    {
		      /* Handle the pushed elements now.  */
		      size_t backw;

		      for (backw = idxcnt - 1; backw >= backw_stop; --backw)
			{
			  len = weights[idxarr[backw]++];

			  if (needed + len < n)
			    while (len-- > 0)
			      dest[needed++] = weights[idxarr[backw]++];
			  else
			    {
				/* No more characters fit into the buffer.  */
			      needed += len;
			      idxarr[backw] += len;
			    }
			}

		      backw_stop = ~0ul;
		    }

		  /* Now handle the forward element.  */
		  len = weights[idxarr[idxcnt]++];
		  if (needed + len < n)
		    while (len-- > 0)
		      dest[needed++] = weights[idxarr[idxcnt]++];
		  else
		    {
		      /* No more characters fit into the buffer.  */
		      needed += len;
		      idxarr[idxcnt] += len;
		    }
		}
	      else
		{
		  /* Remember where the backwards series started.  */
		  if (backw_stop == ~0ul)
		    backw_stop = idxcnt;
		}

	      rule = rulesets[rulearr[idxcnt + 1] * nrules + pass];
	    }


	  if (backw_stop != ~0ul)
	    {
	      /* Handle the pushed elements now.  */
	      size_t backw;

	      backw = idxcnt;
	      while (backw > backw_stop)
		{
		  size_t len = weights[idxarr[--backw]++];

		  if (needed + len < n)
		    while (len-- > 0)
		      dest[needed++] = weights[idxarr[backw]++];
		  else
		    {
		      /* No more characters fit into the buffer.  */
		      needed += len;
		      idxarr[backw] += len;
		    }
		}
	    }
	}
      else
	{
	  int val = 1;
#ifndef WIDE_CHAR_VERSION
	  char buf[7];
	  size_t buflen;
#endif
	  size_t i;

	  for (idxcnt = 0; idxcnt < idxmax; ++idxcnt)
	    {
	      if ((rule & sort_forward) != 0)
		{
		  size_t len;

		  if (backw_stop != ~0ul)
		    {
		     /* Handle the pushed elements now.  */
		      size_t backw;

		      for (backw = idxcnt - 1; backw >= backw_stop; --backw)
			{
			  len = weights[idxarr[backw]++];
			  if (len != 0)
			    {
#ifdef WIDE_CHAR_VERSION
			      if (needed + 1 + len < n)
				{
				  dest[needed] = val;
				  for (i = 0; i < len; ++i)
				    dest[needed + 1 + i] =
				      weights[idxarr[backw] + i];
				}
			      needed += 1 + len;
#else
			      buflen = utf8_encode (buf, val);
			      if (needed + buflen + len < n)
				{
				  for (i = 0; i < buflen; ++i)
				    dest[needed + i] = buf[i];
				  for (i = 0; i < len; ++i)
				    dest[needed + buflen + i] =
				      weights[idxarr[backw] + i];
				}
			      needed += buflen + len;
#endif
			      idxarr[backw] += len;
			      val = 1;
			    }
			  else
			    ++val;
			}

		      backw_stop = ~0ul;
		    }

		  /* Now handle the forward element.  */
		  len = weights[idxarr[idxcnt]++];
		  if (len != 0)
		    {
#ifdef WIDE_CHAR_VERSION
		      if (needed + 1+ len < n)
			{
			  dest[needed] = val;
			  for (i = 0; i < len; ++i)
			    dest[needed + 1 + i] =
			      weights[idxarr[idxcnt] + i];
			}
		      needed += 1 + len;
#else
		      buflen = utf8_encode (buf, val);
		      if (needed + buflen + len < n)
			{
			  for (i = 0; i < buflen; ++i)
			    dest[needed + i] = buf[i];
			  for (i = 0; i < len; ++i)
			    dest[needed + buflen + i] =
			      weights[idxarr[idxcnt] + i];
			}
		      needed += buflen + len;
#endif
		      idxarr[idxcnt] += len;
		      val = 1;
		    }
		  else
		    /* Note that we don't have to increment `idxarr[idxcnt]'
		       since the length is zero.  */
		    ++val;
		}
	      else
		{
		  /* Remember where the backwards series started.  */
		  if (backw_stop == ~0ul)
		    backw_stop = idxcnt;
		}

	      rule = rulesets[rulearr[idxcnt + 1] * nrules + pass];
	    }

	  if (backw_stop != ~0ul)
	    {
	      /* Handle the pushed elements now.  */
	      size_t backw;

	      backw = idxmax - 1;
	      while (backw > backw_stop)
		{
		  size_t len = weights[idxarr[--backw]++];
		  if (len != 0)
		    {
#ifdef WIDE_CHAR_VERSION
		      if (needed + 1 + len < n)
			{
			  dest[needed] = val;
			  for (i = 0; i < len; ++i)
			    dest[needed + 1 + i] =
			      weights[idxarr[backw] + i];
			}
		      needed += 1 + len;
#else
		      buflen = utf8_encode (buf, val);
		      if (needed + buflen + len < n)
			{
			  for (i = 0; i < buflen; ++i)
			    dest[needed + i] = buf[i];
			  for (i = 0; i < len; ++i)
			    dest[needed + buflen + i] =
			      weights[idxarr[backw] + i];
			}
		      needed += buflen + len;
#endif
		      idxarr[backw] += len;
		      val = 1;
		    }
		  else
		    ++val;
		}
	    }
	}

      /* Finally store the byte to separate the passes or terminate
	 the string.  */
      if (needed < n)
	dest[needed] = pass + 1 < nrules ? L('\1') : L('\0');
      ++needed;
    }

  /* This is a little optimization: many collation specifications have
     a `position' rule at the end and if no non-ignored character
     is found the last \1 byte is immediately followed by a \0 byte
     signalling this.  We can avoid the \1 byte(s).  */
  if (needed <= n && needed > 2 && dest[needed - 2] == L('\1'))
    {
      /* Remove the \1 byte.  */
      --needed;
      dest[needed - 1] = L('\0');
    }

  /* Free the memory if needed.  */
  if (use_malloc)
    free (idxarr);

  /* Return the number of bytes/words we need, but don't count the NUL
     byte/word at the end.  */
  return needed - 1;
}
