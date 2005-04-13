/* Transliteration using the locale's data.
   Copyright (C) 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 2000.

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
#include <dlfcn.h>
#include <search.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <bits/libc-lock.h>
#include "gconv_int.h"
#include "../locale/localeinfo.h"


int
__gconv_transliterate (struct __gconv_step *step,
		       struct __gconv_step_data *step_data,
		       void *trans_data __attribute__ ((unused)),
		       const unsigned char *inbufstart,
		       const unsigned char **inbufp,
		       const unsigned char *inbufend,
		       unsigned char **outbufstart, size_t *irreversible)
{
  /* Find out about the locale's transliteration.  */
  uint_fast32_t size;
  const uint32_t *from_idx;
  const uint32_t *from_tbl;
  const uint32_t *to_idx;
  const uint32_t *to_tbl;
  const uint32_t *winbuf;
  const uint32_t *winbufend;
  uint_fast32_t low;
  uint_fast32_t high;

  /* The input buffer.  There are actually 4-byte values.  */
  winbuf = (const uint32_t *) *inbufp;
  winbufend = (const uint32_t *) inbufend;

  /* If there is no transliteration information in the locale don't do
     anything and return the error.  */
  size = _NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_TRANSLIT_TAB_SIZE);
  if (size == 0)
    goto no_rules;

  /* Get the rest of the values.  */
  from_idx =
    (const uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TRANSLIT_FROM_IDX);
  from_tbl =
    (const uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TRANSLIT_FROM_TBL);
  to_idx =
    (const uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TRANSLIT_TO_IDX);
  to_tbl =
    (const uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TRANSLIT_TO_TBL);

  /* Test whether there is enough input.  */
  if (winbuf + 1 > winbufend)
    return (winbuf == winbufend
	    ? __GCONV_EMPTY_INPUT : __GCONV_INCOMPLETE_INPUT);

  /* The array starting at FROM_IDX contains indeces to the string table
     in FROM_TBL.  The indeces are sorted wrt to the strings.  I.e., we
     are doing binary search.  */
  low = 0;
  high = size;
  while (low < high)
    {
      uint_fast32_t med = (low + high) / 2;
      uint32_t idx;
      int cnt;

      /* Compare the string at this index with the string at the current
	 position in the input buffer.  */
      idx = from_idx[med];
      cnt = 0;
      do
	{
	  if (from_tbl[idx + cnt] != winbuf[cnt])
	    /* Does not match.  */
	    break;
	  ++cnt;
	}
      while (from_tbl[idx + cnt] != L'\0' && winbuf + cnt < winbufend);

      if (cnt > 0 && from_tbl[idx + cnt] == L'\0')
	{
	  /* Found a matching input sequence.  Now try to convert the
	     possible replacements.  */
	  uint32_t idx2 = to_idx[med];

	  do
	    {
	      /* Determine length of replacement.  */
	      uint_fast32_t len = 0;
	      int res;
	      const unsigned char *toinptr;
	      unsigned char *outptr;

	      while (to_tbl[idx2 + len] != L'\0')
		++len;

	      /* Try this input text.  */
	      toinptr = (const unsigned char *) &to_tbl[idx2];
	      outptr = *outbufstart;
	      res = DL_CALL_FCT (step->__fct,
				 (step, step_data, &toinptr,
				  (const unsigned char *) &to_tbl[idx2 + len],
				  &outptr, NULL, 0, 0));
	      if (res != __GCONV_ILLEGAL_INPUT)
		{
		  /* If the conversion succeeds we have to increment the
		     input buffer.  */
		  if (res == __GCONV_EMPTY_INPUT)
		    {
		      *inbufp += cnt * sizeof (uint32_t);
		      ++*irreversible;
		      res = __GCONV_OK;
		    }
		  *outbufstart = outptr;

		  return res;
		}

	      /* Next replacement.  */
	      idx2 += len + 1;
	    }
	  while (to_tbl[idx2] != L'\0');

	  /* Nothing found, continue searching.  */
	}
      else if (cnt > 0)
	/* This means that the input buffer contents matches a prefix of
	   an entry.  Since we cannot match it unless we get more input,
	   we will tell the caller about it.  */
	return __GCONV_INCOMPLETE_INPUT;

      if (winbuf + cnt >= winbufend || from_tbl[idx + cnt] < winbuf[cnt])
	low = med + 1;
      else
	high = med;
    }

 no_rules:
  /* Maybe the character is supposed to be ignored.  */
  if (_NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_TRANSLIT_IGNORE_LEN) != 0)
    {
      int n = _NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_TRANSLIT_IGNORE_LEN);
      const uint32_t *ranges =
	(const uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TRANSLIT_IGNORE);
      const uint32_t wc = *(const uint32_t *) (*inbufp);
      int i;

      /* Test whether there is enough input.  */
      if (winbuf + 1 > winbufend)
	return (winbuf == winbufend
		? __GCONV_EMPTY_INPUT : __GCONV_INCOMPLETE_INPUT);

      for (i = 0; i < n; ranges += 3, ++i)
	if (ranges[0] <= wc && wc <= ranges[1]
	    && (wc - ranges[0]) % ranges[2] == 0)
	  {
	    /* Matches the range.  Ignore it.  */
	    *inbufp += 4;
	    ++*irreversible;
	    return __GCONV_OK;
	  }
	else if (wc < ranges[0])
	  /* There cannot be any other matching range since they are
             sorted.  */
	  break;
    }

  /* One last chance: use the default replacement.  */
  if (_NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_TRANSLIT_DEFAULT_MISSING_LEN) != 0)
    {
      const uint32_t *default_missing = (const uint32_t *)
	_NL_CURRENT (LC_CTYPE, _NL_CTYPE_TRANSLIT_DEFAULT_MISSING);
      const unsigned char *toinptr = (const unsigned char *) default_missing;
      uint32_t len = _NL_CURRENT_WORD (LC_CTYPE,
				       _NL_CTYPE_TRANSLIT_DEFAULT_MISSING_LEN);
      unsigned char *outptr;
      int res;

      /* Test whether there is enough input.  */
      if (winbuf + 1 > winbufend)
	return (winbuf == winbufend
		? __GCONV_EMPTY_INPUT : __GCONV_INCOMPLETE_INPUT);

      outptr = *outbufstart;
      res = DL_CALL_FCT (step->__fct,
			 (step, step_data, &toinptr,
			  (const unsigned char *) (default_missing + len),
			  &outptr, NULL, 0, 0));

      if (res != __GCONV_ILLEGAL_INPUT)
	{
	  /* If the conversion succeeds we have to increment the
	     input buffer.  */
	  if (res == __GCONV_EMPTY_INPUT)
	    {
	      /* This worked but is not reversible.  */
	      ++*irreversible;
	      *inbufp += 4;
	      res = __GCONV_OK;
	    }
	  *outbufstart = outptr;

	  return res;
	}
    }

  /* Haven't found a match.  */
  return __GCONV_ILLEGAL_INPUT;
}


/* Structure to represent results of found (or not) transliteration
   modules.  */
struct known_trans
{
  /* This structure must remain the first member.  */
  struct trans_struct info;

  char *fname;
  void *handle;
  int open_count;
};


/* Tree with results of previous calls to __gconv_translit_find.  */
static void *search_tree;

/* We modify global data.   */
__libc_lock_define_initialized (static, lock);


/* Compare two transliteration entries.  */
static int
trans_compare (const void *p1, const void *p2)
{
  const struct known_trans *s1 = (const struct known_trans *) p1;
  const struct known_trans *s2 = (const struct known_trans *) p2;

  return strcmp (s1->info.name, s2->info.name);
}


/* Open (maybe reopen) the module named in the struct.  Get the function
   and data structure pointers we need.  */
static int
open_translit (struct known_trans *trans)
{
  __gconv_trans_query_fct queryfct;

  trans->handle = __libc_dlopen (trans->fname);
  if (trans->handle == NULL)
    /* Not available.  */
    return 1;

  /* Find the required symbol.  */
  queryfct = __libc_dlsym (trans->handle, "gconv_trans_context");
  if (queryfct == NULL)
    {
      /* We cannot live with that.  */
    close_and_out:
      __libc_dlclose (trans->handle);
      trans->handle = NULL;
      return 1;
    }

  /* Get the context.  */
  if (queryfct (trans->info.name, &trans->info.csnames, &trans->info.ncsnames)
      != 0)
    goto close_and_out;

  /* Of course we also have to have the actual function.  */
  trans->info.trans_fct = __libc_dlsym (trans->handle, "gconv_trans");
  if (trans->info.trans_fct == NULL)
    goto close_and_out;

  /* Now the optional functions.  */
  trans->info.trans_init_fct =
    __libc_dlsym (trans->handle, "gconv_trans_init");
  trans->info.trans_context_fct =
    __libc_dlsym (trans->handle, "gconv_trans_context");
  trans->info.trans_end_fct =
    __libc_dlsym (trans->handle, "gconv_trans_end");

  trans->open_count = 1;

  return 0;
}


int
internal_function
__gconv_translit_find (struct trans_struct *trans)
{
  struct known_trans **found;
  const struct path_elem *runp;
  int res = 1;

  /* We have to have a name.  */
  assert (trans->name != NULL);

  /* Acquire the lock.  */
  __libc_lock_lock (lock);

  /* See whether we know this module already.  */
  found = __tfind (trans, &search_tree, trans_compare);
  if (found != NULL)
    {
      /* Is this module available?  */
      if ((*found)->handle != NULL)
	{
	  /* Maybe we have to reopen the file.  */
	  if ((*found)->handle != (void *) -1)
	    /* The object is not unloaded.  */
	    res = 0;
	  else if (open_translit (*found) == 0)
	    {
	      /* Copy the data.  */
	      *trans = (*found)->info;
	      (*found)->open_count++;
	      res = 0;
	    }
	}
    }
  else
    {
      size_t name_len = strlen (trans->name) + 1;
      int need_so = 0;
      struct known_trans *newp;

      /* We have to continue looking for the module.  */
      if (__gconv_path_elem == NULL)
	__gconv_get_path ();

      /* See whether we have to append .so.  */
      if (name_len <= 4 || memcmp (&trans->name[name_len - 4], ".so", 3) != 0)
	need_so = 1;

      /* Create a new entry.  */
      newp = (struct known_trans *) malloc (sizeof (struct known_trans)
					    + (__gconv_max_path_elem_len
					       + name_len + 3)
					    + name_len);
      if (newp != NULL)
	{
	  char *cp;

	  /* Clear the struct.  */
	  memset (newp, '\0', sizeof (struct known_trans));

	  /* Store a copy of the module name.  */
	  newp->info.name = cp = (char *) (newp + 1);
	  cp = __mempcpy (cp, trans->name, name_len);

	  newp->fname = cp;

	  /* Search in all the directories.  */
	  for (runp = __gconv_path_elem; runp->name != NULL; ++runp)
	    {
	      cp = __mempcpy (__stpcpy ((char *) newp->fname, runp->name),
			      trans->name, name_len);
	      if (need_so)
		memcpy (cp, ".so", sizeof (".so"));

	      if (open_translit (newp) == 0)
		{
		  /* We found a module.  */
		  res = 0;
		  break;
		}
	    }

	  if (res)
	    newp->fname = NULL;

	  /* In any case we'll add the entry to our search tree.  */
	  if (__tsearch (newp, &search_tree, trans_compare) == NULL)
	    {
	      /* Yickes, this should not happen.  Unload the object.  */
	      res = 1;
	      /* XXX unload here.  */
	    }
	}
    }

  __libc_lock_unlock (lock);

  return res;
}
