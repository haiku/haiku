/*  GNU SED, a batch stream editor.
    Copyright (C) 1999, 2002, 2003 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include "sed.h"

#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef gettext_noop
# define N_(String) gettext_noop(String)
#else
# define N_(String) (String)
#endif

extern flagT use_extended_syntax_p;

static const char errors[] =
  "No previous regular expression\0"
  "Cannot specify modifiers on empty regexp";

#define NO_REGEX (errors)
#define BAD_MODIF (NO_REGEX + sizeof(N_("No previous regular expression")))
#define END_ERRORS (BAD_MODIF + sizeof(N_("Cannot specify modifiers on empty regexp")))



regex_t *
compile_regex(b, flags, needed_sub)
  struct buffer *b;
  int flags;
  int needed_sub;
{
  regex_t *new_regex;

  char *last_re = NULL;
  size_t last_re_len;

  /* // matches the last RE */
  if (size_buffer(b) == 0)
    {
      if (flags > 0)
	bad_prog(_(BAD_MODIF));
      return NULL;
    }

  last_re_len = size_buffer(b);
  last_re = ck_memdup(get_buffer(b), last_re_len);

  new_regex = MALLOC(1, regex_t);

#ifdef REG_PERL
  {
    int errcode;
    errcode = regncomp(new_regex, last_re, last_re_len,
		       (needed_sub ? 0 : REG_NOSUB)
		       | flags
		       | extended_regexp_flags);

    if (errcode)
      {
        char errorbuf[200];
        regerror(errcode, NULL, errorbuf, 200);
        bad_prog(gettext(errorbuf));
      }
  }
#else
  new_regex->fastmap = malloc (1 << (sizeof (char) * 8));
  {
    const char *error;
    int syntax = ((extended_regexp_flags & REG_EXTENDED)
		   ? RE_SYNTAX_POSIX_EXTENDED
                   : RE_SYNTAX_POSIX_BASIC)
		   & ~RE_UNMATCHED_RIGHT_PAREN_ORD;

    syntax |= RE_NO_POSIX_BACKTRACKING;
#ifdef RE_ICASE
    syntax |= (flags & REG_ICASE) ? RE_ICASE : 0;
#endif

    /* If REG_NEWLINE is set, newlines are treated differently.  */
    if (flags & REG_NEWLINE)
      { /* REG_NEWLINE implies neither . nor [^...] match newline.  */
        syntax &= ~RE_DOT_NEWLINE;
        syntax |= RE_HAT_LISTS_NOT_NEWLINE;
      }

    /* GNU regex does not process \t & co. */
    last_re_len = normalize_text(last_re, last_re_len);
    re_set_syntax (syntax);
    error = re_compile_pattern (last_re, last_re_len, new_regex);
    new_regex->newline_anchor = (flags & REG_NEWLINE) != 0;

    new_regex->translate = NULL;
#ifndef RE_ICASE
    if (flags & REG_ICASE)
      {
        static char translate[1 << (sizeof(char) * 8)];
	int i;
	for (i = 0; i < sizeof(translate) / sizeof(char); i++)
	  translate[i] = tolower (i);

        new_regex->translate = translate;
      }
#endif

    if (error)
      bad_prog(error);
  }
#endif

  FREE(last_re);

  /* Just to be sure, I mark this as not POSIXLY_CORRECT behavior */
  if (new_regex->re_nsub < needed_sub && !POSIXLY_CORRECT)
    {
      char buf[200];
      sprintf(buf, _("Invalid reference \\%d on `s' command's RHS"),
	      needed_sub);
      bad_prog(buf);
    }

  return new_regex;
}

#ifdef REG_PERL
static void
copy_regs (regs, pmatch, nregs)
     struct re_registers *regs;
     regmatch_t *pmatch;
     int nregs;
{
  int i;
  int need_regs = nregs + 1;
  /* We need one extra element beyond `num_regs' for the `-1' marker GNU code
     uses.  */

  /* Have the register data arrays been allocated?  */
  if (!regs->start)
    { /* No.  So allocate them with malloc.  */
      regs->start = MALLOC (need_regs, regoff_t);
      regs->end = MALLOC (need_regs, regoff_t);
      regs->num_regs = need_regs;
    }
  else if (need_regs > regs->num_regs)
    { /* Yes.  We also need more elements than were already
         allocated, so reallocate them.  */
      regs->start = REALLOC (regs->start, need_regs, regoff_t);
      regs->end = REALLOC (regs->end, need_regs, regoff_t);
      regs->num_regs = need_regs;
    }

  /* Copy the regs.  */
  for (i = 0; i < nregs; ++i)
    {
      regs->start[i] = pmatch[i].rm_so;
      regs->end[i] = pmatch[i].rm_eo;
    }
  for ( ; i < regs->num_regs; ++i)
    regs->start[i] = regs->end[i] = -1;
}
#endif

int
match_regex(regex, buf, buflen, buf_start_offset, regarray, regsize)
  regex_t *regex;
  char *buf;
  size_t buflen;
  size_t buf_start_offset;
  struct re_registers *regarray;
  int regsize;
{
  int ret;
  static regex_t *regex_last;
#ifdef REG_PERL
  regmatch_t *regmatch;
  if (regsize)
    regmatch = (regmatch_t *) alloca (sizeof (regmatch_t) * regsize);
  else
    regmatch = NULL;
#endif

  /* printf ("Matching from %d/%d\n", buf_start_offset, buflen); */

  /* Keep track of the last regexp matched. */
  if (!regex)
    {
      regex = regex_last;
      if (!regex_last)
	bad_prog(_(NO_REGEX));
    }
  else
    regex_last = regex;

#ifdef REG_PERL
  ret = regexec2(regex, 
		 buf, CAST(int)buflen, CAST(int)buf_start_offset,
		 regsize, regmatch, 0);

  if (regsize)
    copy_regs (regarray, regmatch, regsize);

  return (ret == 0);
#else
  regex->regs_allocated = REGS_REALLOCATE;
  ret = re_search(regex, buf, buflen, buf_start_offset,
		  buflen - buf_start_offset,
		  regsize ? regarray : NULL);

  return (ret > -1);
#endif
}


#ifdef DEBUG_LEAKS
void
release_regex(regex)
  regex_t *regex;
{
  regfree(regex);
  FREE(regex);
}
#endif /*DEBUG_LEAKS*/
