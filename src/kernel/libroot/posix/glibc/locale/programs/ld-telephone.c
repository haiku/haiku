/* Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <error.h>
#include <langinfo.h>
#include <string.h>
#include <sys/uio.h>

#include <assert.h>

#include "localedef.h"
#include "localeinfo.h"
#include "locfile.h"


/* The real definition of the struct for the LC_TELEPHONE locale.  */
struct locale_telephone_t
{
  const char *tel_int_fmt;
  const char *tel_dom_fmt;
  const char *int_select;
  const char *int_prefix;
};


static void
telephone_startup (struct linereader *lr, struct localedef_t *locale,
		   int ignore_content)
{
  if (!ignore_content)
    locale->categories[LC_TELEPHONE].telephone = (struct locale_telephone_t *)
      xcalloc (1, sizeof (struct locale_telephone_t));

  if (lr != NULL)
    {
      lr->translate_strings = 1;
      lr->return_widestr = 0;
    }
}


void
telephone_finish (struct localedef_t *locale, const struct charmap_t *charmap)
{
  struct locale_telephone_t *telephone =
    locale->categories[LC_TELEPHONE].telephone;
  int nothing = 0;

  /* Now resolve copying and also handle completely missing definitions.  */
  if (telephone == NULL)
    {
      /* First see whether we were supposed to copy.  If yes, find the
	 actual definition.  */
      if (locale->copy_name[LC_TELEPHONE] != NULL)
	{
	  /* Find the copying locale.  This has to happen transitively since
	     the locale we are copying from might also copying another one.  */
	  struct localedef_t *from = locale;

	  do
	    from = find_locale (LC_TELEPHONE, from->copy_name[LC_TELEPHONE],
				from->repertoire_name, charmap);
	  while (from->categories[LC_TELEPHONE].telephone == NULL
		 && from->copy_name[LC_TELEPHONE] != NULL);

	  telephone = locale->categories[LC_TELEPHONE].telephone
	    = from->categories[LC_TELEPHONE].telephone;
	}

      /* If there is still no definition issue an warning and create an
	 empty one.  */
      if (telephone == NULL)
	{
	  if (! be_quiet)
	    WITH_CUR_LOCALE (error (0, 0, _("\
No definition for %s category found"), "LC_TELEPHONE"));
	  telephone_startup (NULL, locale, 0);
	  telephone = locale->categories[LC_TELEPHONE].telephone;
	  nothing = 1;
	}
    }

  if (telephone->tel_int_fmt == NULL)
    {
      if (! nothing)
	WITH_CUR_LOCALE (error (0, 0, _("%s: field `%s' not defined"),
				"LC_TELEPHONE", "tel_int_fmt"));
      /* Use as the default value the value of the i18n locale.  */
      telephone->tel_int_fmt = "+%c %a %l";
    }
  else
    {
      /* We must check whether the format string contains only the
	 allowed escape sequences.  */
      const char *cp = telephone->tel_int_fmt;

      if (*cp == '\0')
	WITH_CUR_LOCALE (error (0, 0, _("%s: field `%s' must not be empty"),
				"LC_TELEPHONE", "tel_int_fmt"));
      else
	while (*cp != '\0')
	  {
	    if (*cp == '%')
	      {
		if (strchr ("aAlc", *++cp) == NULL)
		  {
		    WITH_CUR_LOCALE (error (0, 0, _("\
%s: invalid escape sequence in field `%s'"), "LC_TELEPHONE", "tel_int_fmt"));
		    break;
		  }
	      }
	    ++cp;
	  }
    }

  if (telephone->tel_dom_fmt == NULL)
    telephone->tel_dom_fmt = "";
  else if (telephone->tel_dom_fmt[0] != '\0')
    {
      /* We must check whether the format string contains only the
	 allowed escape sequences.  */
      const char *cp = telephone->tel_dom_fmt;

      while (*cp != '\0')
	{
	  if (*cp == '%')
	    {
	      if (strchr ("aAlc", *++cp) == NULL)
		{
		  WITH_CUR_LOCALE (error (0, 0, _("\
%s: invalid escape sequence in field `%s'"), "LC_TELEPHONE", "tel_dom_fmt"));
		  break;
		}
	    }
	  ++cp;
	}
    }

#define TEST_ELEM(cat) \
  if (telephone->cat == NULL)						      \
    {									      \
      if (verbose && ! nothing)						      \
	WITH_CUR_LOCALE (error (0, 0, _("%s: field `%s' not defined"),	      \
				"LC_TELEPHONE", #cat));     		      \
      telephone->cat = "";						      \
    }

  TEST_ELEM (int_select);
  TEST_ELEM (int_prefix);
}


void
telephone_output (struct localedef_t *locale, const struct charmap_t *charmap,
		  const char *output_path)
{
  struct locale_telephone_t *telephone =
    locale->categories[LC_TELEPHONE].telephone;
  struct iovec iov[2 + _NL_ITEM_INDEX (_NL_NUM_LC_TELEPHONE)];
  struct locale_file data;
  uint32_t idx[_NL_ITEM_INDEX (_NL_NUM_LC_TELEPHONE)];
  size_t cnt = 0;

  data.magic = LIMAGIC (LC_TELEPHONE);
  data.n = _NL_ITEM_INDEX (_NL_NUM_LC_TELEPHONE);
  iov[cnt].iov_base = (void *) &data;
  iov[cnt].iov_len = sizeof (data);
  ++cnt;

  iov[cnt].iov_base = (void *) idx;
  iov[cnt].iov_len = sizeof (idx);
  ++cnt;

  idx[cnt - 2] = iov[0].iov_len + iov[1].iov_len;
  iov[cnt].iov_base = (void *) telephone->tel_int_fmt;
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) telephone->tel_dom_fmt;
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) telephone->int_select;
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) telephone->int_prefix;
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  idx[cnt - 2] = idx[cnt - 3] + iov[cnt - 1].iov_len;
  iov[cnt].iov_base = (void *) charmap->code_set_name;;
  iov[cnt].iov_len = strlen (iov[cnt].iov_base) + 1;
  ++cnt;

  assert (cnt == 2 + _NL_ITEM_INDEX (_NL_NUM_LC_TELEPHONE));

  write_locale_data (output_path, LC_TELEPHONE, "LC_TELEPHONE",
		     2 + _NL_ITEM_INDEX (_NL_NUM_LC_TELEPHONE), iov);
}


/* The parser for the LC_TELEPHONE section of the locale definition.  */
void
telephone_read (struct linereader *ldfile, struct localedef_t *result,
		const struct charmap_t *charmap, const char *repertoire_name,
		int ignore_content)
{
  struct locale_telephone_t *telephone;
  struct token *now;
  struct token *arg;
  enum token_t nowtok;

  /* The rest of the line containing `LC_TELEPHONE' must be free.  */
  lr_ignore_rest (ldfile, 1);

  do
    {
      now = lr_token (ldfile, charmap, result, NULL, verbose);
      nowtok = now->tok;
    }
  while (nowtok == tok_eol);

  /* If we see `copy' now we are almost done.  */
  if (nowtok == tok_copy)
    {
      handle_copy (ldfile, charmap, repertoire_name, result, tok_lc_telephone,
		   LC_TELEPHONE, "LC_TELEPHONE", ignore_content);
      return;
    }

  /* Prepare the data structures.  */
  telephone_startup (ldfile, result, ignore_content);
  telephone = result->categories[LC_TELEPHONE].telephone;

  while (1)
    {
      /* Of course we don't proceed beyond the end of file.  */
      if (nowtok == tok_eof)
	break;

      /* Ingore empty lines.  */
      if (nowtok == tok_eol)
	{
	  now = lr_token (ldfile, charmap, result, NULL, verbose);
	  nowtok = now->tok;
	  continue;
	}

      switch (nowtok)
	{
#define STR_ELEM(cat) \
	case tok_##cat:							      \
	  /* Ignore the rest of the line if we don't need the input of	      \
	     this line.  */						      \
	  if (ignore_content)						      \
	    {								      \
	      lr_ignore_rest (ldfile, 0);				      \
	      break;							      \
	    }								      \
									      \
	  arg = lr_token (ldfile, charmap, result, NULL, verbose);	      \
	  if (arg->tok != tok_string)					      \
	    goto err_label;						      \
	  if (telephone->cat != NULL)					      \
	    lr_error (ldfile, _("%s: field `%s' declared more than once"),    \
		      "LC_TELEPHONE", #cat);				      \
	  else if (!ignore_content && arg->val.str.startmb == NULL)	      \
	    {								      \
	      lr_error (ldfile, _("%s: unknown character in field `%s'"),     \
			"LC_TELEPHONE", #cat);				      \
	      telephone->cat = "";					      \
	    }								      \
	  else if (!ignore_content)					      \
	    telephone->cat = arg->val.str.startmb;			      \
	  break

	  STR_ELEM (tel_int_fmt);
	  STR_ELEM (tel_dom_fmt);
	  STR_ELEM (int_select);
	  STR_ELEM (int_prefix);

	case tok_end:
	  /* Next we assume `LC_TELEPHONE'.  */
	  arg = lr_token (ldfile, charmap, result, NULL, verbose);
	  if (arg->tok == tok_eof)
	    break;
	  if (arg->tok == tok_eol)
	    lr_error (ldfile, _("%s: incomplete `END' line"), "LC_TELEPHONE");
	  else if (arg->tok != tok_lc_telephone)
	    lr_error (ldfile, _("\
%1$s: definition does not end with `END %1$s'"), "LC_TELEPHONE");
	  lr_ignore_rest (ldfile, arg->tok == tok_lc_telephone);
	  return;

	default:
	err_label:
	  SYNTAX_ERROR (_("%s: syntax error"), "LC_TELEPHONE");
	}

      /* Prepare for the next round.  */
      now = lr_token (ldfile, charmap, result, NULL, verbose);
      nowtok = now->tok;
    }

  /* When we come here we reached the end of the file.  */
  lr_error (ldfile, _("%s: premature end of file"), "LC_TELEPHONE");
}
