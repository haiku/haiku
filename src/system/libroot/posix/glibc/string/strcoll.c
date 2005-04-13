/* Copyright (C) 1995,96,97,98,99,2000,2001,2002 Free Software Foundation, Inc.
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

#ifndef STRING_TYPE
# define STRING_TYPE char
# define USTRING_TYPE unsigned char
# ifdef USE_IN_EXTENDED_LOCALE_MODEL
#  define STRCOLL __strcoll_l
# else
#  define STRCOLL strcoll
# endif
# define STRCMP strcmp
# define STRLEN strlen
# define WEIGHT_H "../locale/weight.h"
# define SUFFIX	MB
# define L(arg) arg
#endif

#define CONCAT(a,b) CONCAT1(a,b)
#define CONCAT1(a,b) a##b

#include "../locale/localeinfo.h"

#ifndef USE_IN_EXTENDED_LOCALE_MODEL
int
STRCOLL (s1, s2)
     const STRING_TYPE *s1;
     const STRING_TYPE *s2;
#else
int
STRCOLL (s1, s2, l)
     const STRING_TYPE *s1;
     const STRING_TYPE *s2;
     __locale_t l;
#endif
{
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
  struct locale_data *current = l->__locales[LC_COLLATE];
  uint_fast32_t nrules = current->values[_NL_ITEM_INDEX (_NL_COLLATE_NRULES)].word;
#else
  uint_fast32_t nrules = _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);
#endif
  /* We don't assign the following values right away since it might be
     unnecessary in case there are no rules.  */
  const unsigned char *rulesets;
  const int32_t *table;
  const USTRING_TYPE *weights;
  const USTRING_TYPE *extra;
  const int32_t *indirect;
  uint_fast32_t pass;
  int result = 0;
  const USTRING_TYPE *us1;
  const USTRING_TYPE *us2;
  size_t s1len;
  size_t s2len;
  int32_t *idx1arr;
  int32_t *idx2arr;
  unsigned char *rule1arr;
  unsigned char *rule2arr;
  size_t idx1max;
  size_t idx2max;
  size_t idx1cnt;
  size_t idx2cnt;
  size_t idx1now;
  size_t idx2now;
  size_t backw1_stop;
  size_t backw2_stop;
  size_t backw1;
  size_t backw2;
  int val1;
  int val2;
  int position;
  int seq1len;
  int seq2len;
  int use_malloc;

#include WEIGHT_H

  if (nrules == 0)
    return STRCMP (s1, s2);

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

  /* We need this a few times.  */
  s1len = STRLEN (s1);
  s2len = STRLEN (s2);

  /* Catch empty strings.  */
  if (__builtin_expect (s1len == 0, 0) || __builtin_expect (s2len == 0, 0))
    return (s1len != 0) - (s2len != 0);

  /* We need the elements of the strings as unsigned values since they
     are used as indeces.  */
  us1 = (const USTRING_TYPE *) s1;
  us2 = (const USTRING_TYPE *) s2;

  /* Perform the first pass over the string and while doing this find
     and store the weights for each character.  Since we want this to
     be as fast as possible we are using `alloca' to store the temporary
     values.  But since there is no limit on the length of the string
     we have to use `malloc' if the string is too long.  We should be
     very conservative here.

     Please note that the localedef programs makes sure that `position'
     is not used at the first level.  */
  if (! __libc_use_alloca (s1len + s2len))
    {
      idx1arr = (int32_t *) malloc ((s1len + s2len) * (sizeof (int32_t) + 1));
      idx2arr = &idx1arr[s1len];
      rule1arr = (unsigned char *) &idx2arr[s2len];
      rule2arr = &rule1arr[s1len];

      if (idx1arr == NULL)
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
      idx1arr = (int32_t *) alloca (s1len * sizeof (int32_t));
      idx2arr = (int32_t *) alloca (s2len * sizeof (int32_t));
      rule1arr = (unsigned char *) alloca (s1len);
      rule2arr = (unsigned char *) alloca (s2len);
    }

  idx1cnt = 0;
  idx2cnt = 0;
  idx1max = 0;
  idx2max = 0;
  idx1now = 0;
  idx2now = 0;
  backw1_stop = ~0ul;
  backw2_stop = ~0ul;
  backw1 = ~0ul;
  backw2 = ~0ul;
  seq1len = 0;
  seq2len = 0;
  position = rulesets[0] & sort_position;
  while (1)
    {
      val1 = 0;
      val2 = 0;

      /* Get the next non-IGNOREd element for string `s1'.  */
      if (seq1len == 0)
	do
	  {
	    ++val1;

	    if (backw1_stop != ~0ul)
	      {
		/* The is something pushed.  */
		if (backw1 == backw1_stop)
		  {
		    /* The last pushed character was handled.  Continue
		       with forward characters.  */
		    if (idx1cnt < idx1max)
		      idx1now = idx1cnt;
		    else
		      /* Nothing anymore.  The backward sequence ended with
			 the last sequence in the string.  Note that seq1len
			 is still zero.  */
		      break;
		  }
		else
		  idx1now = --backw1;
	      }
	    else
	      {
		backw1_stop = idx1max;

		while (*us1 != L('\0'))
		  {
		    int32_t tmp = findidx (&us1);
		    rule1arr[idx1max] = tmp >> 24;
		    idx1arr[idx1max] = tmp & 0xffffff;
		    idx1cnt = idx1max++;

		    if ((rulesets[rule1arr[idx1cnt] * nrules]
			 & sort_backward) == 0)
		      /* No more backward characters to push.  */
		      break;
		    ++idx1cnt;
		  }

		if (backw1_stop >= idx1cnt)
		  {
		    /* No sequence at all or just one.  */
		    if (idx1cnt == idx1max || backw1_stop > idx1cnt)
		      /* Note that seq1len is still zero.  */
		      break;

		    backw1_stop = ~0ul;
		    idx1now = idx1cnt;
		  }
		else
		  /* We pushed backward sequences.  */
		  idx1now = backw1 = idx1cnt - 1;
	      }
	  }
	while ((seq1len = weights[idx1arr[idx1now]++]) == 0);

      /* And the same for string `s2'.  */
      if (seq2len == 0)
	do
	  {
	    ++val2;

	    if (backw2_stop != ~0ul)
	      {
		/* The is something pushed.  */
		if (backw2 == backw2_stop)
		  {
		    /* The last pushed character was handled.  Continue
		       with forward characters.  */
		    if (idx2cnt < idx2max)
		      idx2now = idx2cnt;
		    else
		      /* Nothing anymore.  The backward sequence ended with
			 the last sequence in the string.  Note that seq2len
			 is still zero.  */
		      break;
		  }
		else
		  idx2now = --backw2;
	      }
	    else
	      {
		backw2_stop = idx2max;

		while (*us2 != L('\0'))
		  {
		    int32_t tmp = findidx (&us2);
		    rule2arr[idx2max] = tmp >> 24;
		    idx2arr[idx2max] = tmp & 0xffffff;
		    idx2cnt = idx2max++;

		    if ((rulesets[rule2arr[idx2cnt] * nrules]
			 & sort_backward) == 0)
		      /* No more backward characters to push.  */
		      break;
		    ++idx2cnt;
		  }

		if (backw2_stop >= idx2cnt)
		  {
		    /* No sequence at all or just one.  */
		    if (idx2cnt == idx2max || backw2_stop > idx2cnt)
		      /* Note that seq1len is still zero.  */
		      break;

		    backw2_stop = ~0ul;
		    idx2now = idx2cnt;
		  }
		else
		  /* We pushed backward sequences.  */
		  idx2now = backw2 = idx2cnt - 1;
	      }
	  }
	while ((seq2len = weights[idx2arr[idx2now]++]) == 0);

      /* See whether any or both strings are empty.  */
      if (seq1len == 0 || seq2len == 0)
	{
	  if (seq1len == seq2len)
	    /* Both ended.  So far so good, both strings are equal at the
	       first level.  */
	    break;

	  /* This means one string is shorter than the other.  Find out
	     which one and return an appropriate value.  */
	  result = seq1len == 0 ? -1 : 1;
	  goto free_and_return;
	}

      /* Test for position if necessary.  */
      if (position && val1 != val2)
	{
	  result = val1 - val2;
	  goto free_and_return;
	}

      /* Compare the two sequences.  */
      do
	{
	  if (weights[idx1arr[idx1now]] != weights[idx2arr[idx2now]])
	    {
	      /* The sequences differ.  */
	      result = weights[idx1arr[idx1now]] - weights[idx2arr[idx2now]];
	      goto free_and_return;
	    }

	  /* Increment the offsets.  */
	  ++idx1arr[idx1now];
	  ++idx2arr[idx2now];

	  --seq1len;
	  --seq2len;
	}
      while (seq1len > 0 && seq2len > 0);

      if (position && seq1len != seq2len)
	{
	  result = seq1len - seq2len;
	  goto free_and_return;
	}
    }

  /* Now the remaining passes over the weights.  We now use the
     indeces we found before.  */
  for (pass = 1; pass < nrules; ++pass)
    {
      /* We assume that if a rule has defined `position' in one section
	 this is true for all of them.  */
      idx1cnt = 0;
      idx2cnt = 0;
      backw1_stop = ~0ul;
      backw2_stop = ~0ul;
      backw1 = ~0ul;
      backw2 = ~0ul;
      position = rulesets[rule1arr[0] * nrules + pass] & sort_position;

      while (1)
	{
	  val1 = 0;
	  val2 = 0;

	  /* Get the next non-IGNOREd element for string `s1'.  */
	  if (seq1len == 0)
	    do
	      {
		++val1;

		if (backw1_stop != ~0ul)
		  {
		    /* The is something pushed.  */
		    if (backw1 == backw1_stop)
		      {
			/* The last pushed character was handled.  Continue
			   with forward characters.  */
			if (idx1cnt < idx1max)
			  idx1now = idx1cnt;
			else
			  {
			    /* Nothing anymore.  The backward sequence
			       ended with the last sequence in the string.  */
			    idx1now = ~0ul;
			    break;
			  }
		      }
		    else
		      idx1now = --backw1;
		  }
		else
		  {
		    backw1_stop = idx1cnt;

		    while (idx1cnt < idx1max)
		      {
			if ((rulesets[rule1arr[idx1cnt] * nrules + pass]
			     & sort_backward) == 0)
			  /* No more backward characters to push.  */
			  break;
			++idx1cnt;
		      }

		    if (backw1_stop == idx1cnt)
		      {
			/* No sequence at all or just one.  */
			if (idx1cnt == idx1max)
			  /* Note that seq1len is still zero.  */
			  break;

			backw1_stop = ~0ul;
			idx1now = idx1cnt++;
		      }
		    else
		      /* We pushed backward sequences.  */
		      idx1now = backw1 = idx1cnt - 1;
		  }
	      }
	    while ((seq1len = weights[idx1arr[idx1now]++]) == 0);

	  /* And the same for string `s2'.  */
	  if (seq2len == 0)
	    do
	      {
		++val2;

		if (backw2_stop != ~0ul)
		  {
		    /* The is something pushed.  */
		    if (backw2 == backw2_stop)
		      {
			/* The last pushed character was handled.  Continue
			   with forward characters.  */
			if (idx2cnt < idx2max)
			  idx2now = idx2cnt;
			else
			  {
			    /* Nothing anymore.  The backward sequence
			       ended with the last sequence in the string.  */
			    idx2now = ~0ul;
			    break;
			  }
		      }
		    else
		      idx2now = --backw2;
		  }
		else
		  {
		    backw2_stop = idx2cnt;

		    while (idx2cnt < idx2max)
		      {
			if ((rulesets[rule2arr[idx2cnt] * nrules + pass]
			     & sort_backward) == 0)
			  /* No more backward characters to push.  */
			  break;
			++idx2cnt;
		      }

		    if (backw2_stop == idx2cnt)
		      {
			/* No sequence at all or just one.  */
			if (idx2cnt == idx2max)
			  /* Note that seq2len is still zero.  */
			  break;

			backw2_stop = ~0ul;
			idx2now = idx2cnt++;
		      }
		    else
		      /* We pushed backward sequences.  */
		      idx2now = backw2 = idx2cnt - 1;
		  }
	      }
	    while ((seq2len = weights[idx2arr[idx2now]++]) == 0);

	  /* See whether any or both strings are empty.  */
	  if (seq1len == 0 || seq2len == 0)
	    {
	      if (seq1len == seq2len)
		/* Both ended.  So far so good, both strings are equal
		   at this level.  */
		break;

	      /* This means one string is shorter than the other.  Find out
		 which one and return an appropriate value.  */
	      result = seq1len == 0 ? -1 : 1;
	      goto free_and_return;
	    }

	  /* Test for position if necessary.  */
	  if (position && val1 != val2)
	    {
	      result = val1 - val2;
	      goto free_and_return;
	    }

	  /* Compare the two sequences.  */
	  do
	    {
	      if (weights[idx1arr[idx1now]] != weights[idx2arr[idx2now]])
		{
		  /* The sequences differ.  */
		  result = (weights[idx1arr[idx1now]]
			    - weights[idx2arr[idx2now]]);
		  goto free_and_return;
		}

	      /* Increment the offsets.  */
	      ++idx1arr[idx1now];
	      ++idx2arr[idx2now];

	      --seq1len;
	      --seq2len;
	    }
	  while (seq1len > 0 && seq2len > 0);

	  if (position && seq1len != seq2len)
	    {
	      result = seq1len - seq2len;
	      goto free_and_return;
	    }
	}
    }

  /* Free the memory if needed.  */
 free_and_return:
  if (use_malloc)
    free (idx1arr);

  return result;
}
#if !defined WIDE_CHAR_VERSION && !defined USE_IN_EXTENDED_LOCALE_MODEL
libc_hidden_def (strcoll)
#endif
