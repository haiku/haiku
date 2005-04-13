/* Copyright (C) 1996,1997,1998,1999,2000,2001 Free Software Foundation, Inc.
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

#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _POSIX_MAPPED_FILES
# include <sys/mman.h>
#endif

#include "localeinfo.h"
#include "../iconv/gconv_charset.h"


/* Constant data defined in setlocale.c.  */
extern struct locale_data *const _nl_C[];


/* For each category we keep a list of records for the locale files
   which are somehow addressed.  */
struct loaded_l10nfile *_nl_locale_file_list[__LC_LAST];


struct locale_data *
_nl_find_locale (const char *locale_path, size_t locale_path_len,
		 int category, const char **name)
{
  int mask;
  /* Name of the locale for this category.  */
  char *loc_name;
  const char *language;
  const char *modifier;
  const char *territory;
  const char *codeset;
  const char *normalized_codeset;
  const char *special;
  const char *sponsor;
  const char *revision;
  struct loaded_l10nfile *locale_file;

  if ((*name)[0] == '\0')
    {
      /* The user decides which locale to use by setting environment
	 variables.  */
      *name = getenv ("LC_ALL");
      if (*name == NULL || (*name)[0] == '\0')
	*name = getenv (_nl_category_names[category]);
      if (*name == NULL || (*name)[0] == '\0')
	*name = getenv ("LANG");
    }

  if (*name == NULL || (*name)[0] == '\0'
      || (__builtin_expect (__libc_enable_secure, 0)
	  && strchr (*name, '/') != NULL))
    *name = (char *) _nl_C_name;

  if (__builtin_expect (strcmp (*name, _nl_C_name), 1) == 0
      || __builtin_expect (strcmp (*name, _nl_POSIX_name), 1) == 0)
    {
      /* We need not load anything.  The needed data is contained in
	 the library itself.  */
      *name = (char *) _nl_C_name;
      return _nl_C[category];
    }

  /* We really have to load some data.  First see whether the name is
     an alias.  Please note that this makes it impossible to have "C"
     or "POSIX" as aliases.  */
  loc_name = (char *) _nl_expand_alias (*name);
  if (loc_name == NULL)
    /* It is no alias.  */
    loc_name = (char *) *name;

  /* Make a writable copy of the locale name.  */
  loc_name = strdupa (loc_name);

  /* LOCALE can consist of up to four recognized parts for the XPG syntax:

		language[_territory[.codeset]][@modifier]

     and six parts for the CEN syntax:

	language[_territory][+audience][+special][,[sponsor][_revision]]

     Beside the first all of them are allowed to be missing.  If the
     full specified locale is not found, the less specific one are
     looked for.  The various part will be stripped of according to
     the following order:
		(1) revision
		(2) sponsor
		(3) special
		(4) codeset
		(5) normalized codeset
		(6) territory
		(7) audience/modifier
   */
  mask = _nl_explode_name (loc_name, &language, &modifier, &territory,
			   &codeset, &normalized_codeset, &special,
			   &sponsor, &revision);

  /* If exactly this locale was already asked for we have an entry with
     the complete name.  */
  locale_file = _nl_make_l10nflist (&_nl_locale_file_list[category],
				    locale_path, locale_path_len, mask,
				    language, territory, codeset,
				    normalized_codeset, modifier, special,
				    sponsor, revision,
				    _nl_category_names[category], 0);

  if (locale_file == NULL)
    {
      /* Find status record for addressed locale file.  We have to search
	 through all directories in the locale path.  */
      locale_file = _nl_make_l10nflist (&_nl_locale_file_list[category],
					locale_path, locale_path_len, mask,
					language, territory, codeset,
					normalized_codeset, modifier, special,
					sponsor, revision,
					_nl_category_names[category], 1);
      if (locale_file == NULL)
	/* This means we are out of core.  */
	return NULL;
    }

  /* The space for normalized_codeset is dynamically allocated.  Free it.  */
  if (mask & XPG_NORM_CODESET)
    free ((void *) normalized_codeset);

  if (locale_file->decided == 0)
    _nl_load_locale (locale_file, category);

  if (locale_file->data == NULL)
    {
      int cnt;
      for (cnt = 0; locale_file->successor[cnt] != NULL; ++cnt)
	{
	  if (locale_file->successor[cnt]->decided == 0)
	    _nl_load_locale (locale_file->successor[cnt], category);
	  if (locale_file->successor[cnt]->data != NULL)
	    break;
	}
      /* Move the entry we found (or NULL) to the first place of
	 successors.  */
      locale_file->successor[0] = locale_file->successor[cnt];
      locale_file = locale_file->successor[cnt];

      if (locale_file == NULL)
	return NULL;
    }

  /* The LC_CTYPE category allows to check whether a locale is really
     usable.  If the locale name contains a charset name and the
     charset name used in the locale (present in the LC_CTYPE data) is
     not the same (after resolving aliases etc) we reject the locale
     since using it would irritate users expecting the charset named
     in the locale name.  */
  if (codeset != NULL)
    {
      /* Get the codeset information from the locale file.  */
      static const int codeset_idx[] =
	{
	  [__LC_CTYPE] = _NL_ITEM_INDEX (CODESET),
	  [__LC_NUMERIC] = _NL_ITEM_INDEX (_NL_NUMERIC_CODESET),
	  [__LC_TIME] = _NL_ITEM_INDEX (_NL_TIME_CODESET),
	  [__LC_COLLATE] = _NL_ITEM_INDEX (_NL_COLLATE_CODESET),
	  [__LC_MONETARY] = _NL_ITEM_INDEX (_NL_MONETARY_CODESET),
	  [__LC_MESSAGES] = _NL_ITEM_INDEX (_NL_MESSAGES_CODESET),
	  [__LC_PAPER] = _NL_ITEM_INDEX (_NL_PAPER_CODESET),
	  [__LC_NAME] = _NL_ITEM_INDEX (_NL_NAME_CODESET),
	  [__LC_ADDRESS] = _NL_ITEM_INDEX (_NL_ADDRESS_CODESET),
	  [__LC_TELEPHONE] = _NL_ITEM_INDEX (_NL_TELEPHONE_CODESET),
	  [__LC_MEASUREMENT] = _NL_ITEM_INDEX (_NL_MEASUREMENT_CODESET),
	  [__LC_IDENTIFICATION] = _NL_ITEM_INDEX (_NL_IDENTIFICATION_CODESET)
	};
      const struct locale_data *data;
      const char *locale_codeset;
      char *clocale_codeset;
      char *ccodeset;

      data = (const struct locale_data *) locale_file->data;
      locale_codeset =
	(const char *) data->values[codeset_idx[category]].string;
      assert (locale_codeset != NULL);
      /* Note the length of the allocated memory: +3 for up to two slashes
	 and the NUL byte.  */
      clocale_codeset = (char *) alloca (strlen (locale_codeset) + 3);
      strip (clocale_codeset, locale_codeset);

      ccodeset = (char *) alloca (strlen (codeset) + 3);
      strip (ccodeset, codeset);

      if (__gconv_compare_alias (upstr (ccodeset, ccodeset),
				 upstr (clocale_codeset,
					clocale_codeset)) != 0)
	/* The codesets are not identical, don't use the locale.  */
	return NULL;
    }

  /* Determine the locale name for which loading succeeded.  This
     information comes from the file name.  The form is
     <path>/<locale>/LC_foo.  We must extract the <locale> part.  */
  if (((const struct locale_data *) locale_file->data)->name == NULL)
    {
      char *cp, *endp;

      endp = strrchr (locale_file->filename, '/');
      cp = endp - 1;
      while (cp[-1] != '/')
	--cp;
      ((struct locale_data *) locale_file->data)->name = __strndup (cp,
								    endp - cp);
    }

  /* Determine whether the user wants transliteration or not.  */
  if ((modifier != NULL && __strcasecmp (modifier, "TRANSLIT") == 0)
      || (special != NULL && __strcasecmp (special, "TRANSLIT") == 0))
    ((struct locale_data *) locale_file->data)->use_translit = 1;

  /* Increment the usage count.  */
  if (((const struct locale_data *) locale_file->data)->usage_count
      < MAX_USAGE_COUNT)
    ++((struct locale_data *) locale_file->data)->usage_count;

  return (struct locale_data *) locale_file->data;
}


/* Calling this function assumes the lock for handling global locale data
   is acquired.  */
void
_nl_remove_locale (int locale, struct locale_data *data)
{
  if (--data->usage_count == 0)
    {
      /* First search the entry in the list of loaded files.  */
      struct loaded_l10nfile *ptr = _nl_locale_file_list[locale];

      /* Search for the entry.  It must be in the list.  Otherwise it
	 is a bug and we crash badly.  */
      while ((struct locale_data *) ptr->data != data)
	ptr = ptr->next;

      /* Mark the data as not available anymore.  So when the data has
	 to be used again it is reloaded.  */
      ptr->decided = 0;
      ptr->data = NULL;

      /* Free the name.  */
      free ((char *) data->name);

#ifdef _POSIX_MAPPED_FILES
      /* Really delete the data.  First delete the real data.  */
      if (__builtin_expect (data->mmaped, 1))
	{
	  /* Try to unmap the area.  If this fails we mark the area as
	     permanent.  */
	  if (__munmap ((caddr_t) data->filedata, data->filesize) != 0)
	    {
	      data->usage_count = UNDELETABLE;
	      return;
	    }
	}
      else
#endif	/* _POSIX_MAPPED_FILES */
	/* The memory was malloced.  */
	free ((void *) data->filedata);

      /* Now free the structure itself.  */
      free (data);
    }
}
