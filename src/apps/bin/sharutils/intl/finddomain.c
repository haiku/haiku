/* finddomain.c -- handle list of needed message catalogs
   Copyright (C) 1995 Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

#if defined STDC_HEADERS || defined _LIBC
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# include <string.h>
#else
# include <strings.h>
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#if defined HAVE_UNISTD_H || defined _LIBC
# include <unistd.h>
#endif

#include "gettext.h"
#include "gettextP.h"
#ifdef _LIBC
# include <libintl.h>
#else
# include "libgettext.h"
#endif

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# define stpcpy __stpcpy
#endif

/* Encoding of locale name parts.  */
#define CEN_REVISION	1
#define CEN_SPONSOR	2
#define CEN_SPECIAL	4
#define XPG_CODESET	8
#define TERRITORY	16
#define CEN_AUDIENCE	32
#define XPG_MODIFIER	64

#define CEN_SPECIFIC	(CEN_REVISION|CEN_SPONSOR|CEN_SPECIAL|CEN_AUDIENCE)
#define XPG_SPECIFIC	(XPG_CODESET|XPG_MODIFIER)


/* List of already loaded domains.  */
static struct loaded_domain *_nl_loaded_domains;

/* Prototypes for local functions.  */
static struct loaded_domain *make_entry_rec __P ((const char *dirname,
						  int mask,
						  const char *language,
						  const char *territory,
						  const char *codeset,
						  const char *modifier,
						  const char *special,
						  const char *sponsor,
						  const char *revision,
						  const char *domainname,
						  int do_allocate));

/* Substitution for systems lacking this function in their C library.  */
#if !_LIBC && !HAVE_STPCPY
static char *stpcpy __P ((char *dest, const char *src));
#endif


/* Return a data structure describing the message catalog described by
   the DOMAINNAME and CATEGORY parameters with respect to the currently
   established bindings.  */
struct loaded_domain *
_nl_find_domain (dirname, locale, domainname)
     const char *dirname;
     char *locale;
     const char *domainname;
{
  enum { undecided, xpg, cen } syntax;
  struct loaded_domain *retval;
  const char *language;
  const char *modifier = NULL;
  const char *territory = NULL;
  const char *codeset = NULL;
  const char *special = NULL;
  const char *sponsor = NULL;
  const char *revision = NULL;
  const char *alias_value = NULL;
  char *cp;
  int mask;

  /* CATEGORYVALUE now possibly contains a colon separated list of
     locales.  Each single locale can consist of up to four recognized
     parts for the XPG syntax:

		language[_territory[.codeset]][@modifier]

     and six parts for the CEN syntax:

	language[_territory][+audience][+special][,sponsor][_revision]

     Beside the first all of them are allowed to be missing.  If the
     full specified locale is not found, the less specific one are
     looked for.  The various part will be stripped of according to
     the following order:
		(1) revision
		(2) sponsor
		(3) special
		(4) codeset
		(5) territory
		(6) audience/modifier
   */

  /* If we have already tested for this locale entry there has to
     be one data set in the list of loaded domains.  */
  retval = make_entry_rec (dirname, 0, locale, NULL, NULL, NULL,
			   NULL, NULL, NULL, domainname, 0);
  if (retval != NULL)
    {
      /* We know something about this locale.  */
      int cnt;

      if (retval->decided == 0)
	_nl_load_domain (retval); /* @@@ */

      if (retval->data != NULL)
	return retval;

      for (cnt = 0; retval->successor[cnt] != NULL; ++cnt)
	{
	  if (retval->successor[cnt]->decided == 0)
	    _nl_load_domain (retval->successor[cnt]);

	  if (retval->successor[cnt]->data != NULL)
	    break;
	}

      /* We really found some usable information.  */
      return cnt >= 0 ? retval : NULL;
      /* NOTREACHED */
    }

  /* See whether the locale value is an alias.  If yes its value
     *overwrites* the alias name.  No test for the original value is
     done.  */
  alias_value = _nl_expand_alias (locale);
  if (alias_value != NULL)
    {
      size_t len = strlen (alias_value) + 1;
      locale = (char *) malloc (len);
      if (locale == NULL)
	return NULL;

      memcpy (locale, alias_value, len);
    }

  /* Now we determine the single parts of the locale name.  First
     look for the language.  Termination symbols are `_' and `@' if
     we use XPG4 style, and `_', `+', and `,' if we use CEN syntax.  */
  mask = 0;
  syntax = undecided;
  language = cp = locale;
  while (cp[0] != '\0' && cp[0] != '_' && cp[0] != '@'
	 && cp[0] != '+' && cp[0] != ',')
    ++cp;

  if (language == cp)
    /* This does not make sense: language has to be specified.  Use
       this entry as it is without exploding.  Perhaps it is an alias.  */
    cp = strchr (language, '\0');
  else if (cp[0] == '_')
    {
      /* Next is the territory.  */
      cp[0] = '\0';
      territory = ++cp;

      while (cp[0] != '\0' && cp[0] != '.' && cp[0] != '@'
	     && cp[0] != '+' && cp[0] != ',' && cp[0] != '_')
	++cp;

      mask |= TERRITORY;

      if (cp[0] == '.')
	{
	  /* Next is the codeset.  */
	  syntax = xpg;
	  cp[0] = '\0';
	  codeset = ++cp;

	  while (cp[0] != '\0' && cp[0] != '@')
	    ++cp;

	  mask |= XPG_CODESET;
	}
    }

  if (cp[0] == '@' || (syntax != xpg && cp[0] == '+'))
    {
      /* Next is the modifier.  */
      syntax = cp[0] == '@' ? xpg : cen;
      cp[0] = '\0';
      modifier = ++cp;

      while (syntax == cen && cp[0] != '\0' && cp[0] != '+'
	     && cp[0] != ',' && cp[0] != '_')
	++cp;

      mask |= XPG_MODIFIER | CEN_AUDIENCE;
    }

  if (syntax != xpg && (cp[0] == '+' || cp[0] == ',' || cp[0] == '_'))
    {
      syntax = cen;

      if (cp[0] == '+')
	{
 	  /* Next is special application (CEN syntax).  */
	  cp[0] = '\0';
	  special = ++cp;

	  while (cp[0] != '\0' && cp[0] != ',' && cp[0] != '_')
	    ++cp;

	  mask |= CEN_SPECIAL;
	}

      if (cp[0] == ',')
	{
 	  /* Next is sponsor (CEN syntax).  */
	  cp[0] = '\0';
	  sponsor = ++cp;

	  while (cp[0] != '\0' && cp[0] != '_')
	    ++cp;

	  mask |= CEN_SPONSOR;
	}

      if (cp[0] == '_')
	{
 	  /* Next is revision (CEN syntax).  */
	  cp[0] = '\0';
	  revision = ++cp;

	  mask |= CEN_REVISION;
	}
    }

  /* For CEN sytnax values it might be important to have the
     separator character in the file name, not for XPG syntax.  */
  if (syntax == xpg)
    {
      if (territory != NULL && territory[0] == '\0')
	mask &= ~TERRITORY;

      if (codeset != NULL && codeset[0] == '\0')
	mask &= ~XPG_CODESET;

      if (modifier != NULL && modifier[0] == '\0')
	mask &= ~XPG_MODIFIER;
    }

  /* Create all possible locale entries which might be interested in
     generalzation.  */
  retval = make_entry_rec (dirname, mask, language, territory, codeset,
			   modifier, special, sponsor, revision,
			   domainname, 1);
  if (retval == NULL)
    /* This means we are out of core.  */
    return NULL;

  if (retval->decided == 0)
    _nl_load_domain (retval);
  if (retval->data == NULL)
    {
      int cnt;
      for (cnt = 0; retval->successor[cnt] != NULL; ++cnt)
	{
	  if (retval->successor[cnt]->decided == 0)
	    _nl_load_domain (retval->successor[cnt]);
	  if (retval->successor[cnt]->data != NULL)
	    break;

	  /* Signal that locale is not available.  */
	  retval->successor[cnt] = NULL;
	}
      if (retval->successor[cnt] == NULL)
	retval = NULL;
    }

  /* The room for an alias was dynamically allocated.  Free it now.  */
  if (alias_value != NULL)
    free (locale);

  return retval;
}


static struct loaded_domain *
make_entry_rec (dirname, mask, language, territory, codeset, modifier,
		special, sponsor, revision, domain, do_allocate)
     const char *dirname;
     int mask;
     const char *language;
     const char *territory;
     const char *codeset;
     const char *modifier;
     const char *special;
     const char *sponsor;
     const char *revision;
     const char *domain;
     int do_allocate;
{
  char *filename = NULL;
  struct loaded_domain *last = NULL;
  struct loaded_domain *retval;
  char *cp;
  size_t entries;
  int cnt;


  /* Process the current entry described by the MASK only when it is
     valid.  Because the mask can have in the first call bits from
     both syntaces set this is necessary to prevent constructing
     illegal local names.  */
  /* FIXME: Rewrite because test is necessary only in first round.  */
  if ((mask & CEN_SPECIFIC) == 0 || (mask & XPG_SPECIFIC) == 0)
    {
      /* Allocate room for the full file name.  */
      filename = (char *) malloc (strlen (dirname) + 1
				  + strlen (language)
				  + ((mask & TERRITORY) != 0
				     ? strlen (territory) : 0)
				  + ((mask & XPG_CODESET) != 0
				     ? strlen (codeset) : 0)
				  + ((mask & XPG_MODIFIER) != 0 ?
				     strlen (modifier) : 0)
				  + ((mask & CEN_SPECIAL) != 0
				     ? strlen (special) : 0)
				  + ((mask & CEN_SPONSOR) != 0
				     ? strlen (sponsor) : 0)
				  + ((mask & CEN_REVISION) != 0
				     ? strlen (revision) : 0) + 1
				  + strlen (domain) + 1);

      if (filename == NULL)
	return NULL;

      retval = NULL;
      last = NULL;

      /* Construct file name.  */
      cp = stpcpy (filename, dirname);
      *cp++ = '/';
      cp = stpcpy (cp, language);

      if ((mask & TERRITORY) != 0)
	{
	  *cp++ = '_';
	  cp = stpcpy (cp, territory);
	}
      if ((mask & XPG_CODESET) != 0)
	{
	  *cp++ = '.';
      cp = stpcpy (cp, codeset);
	}
      if ((mask & (XPG_MODIFIER | CEN_AUDIENCE)) != 0)
	{
	  /* This component can be part of both syntaces but has different
	     leading characters.  For CEN we use `+', else `@'.  */
	  *cp++ = (mask & CEN_AUDIENCE) != 0 ? '+' : '@';
	  cp = stpcpy (cp, modifier);
	}
      if ((mask & CEN_SPECIAL) != 0)
	{
	  *cp++ = '+';
	  cp = stpcpy (cp, special);
	}
      if ((mask & CEN_SPONSOR) != 0)
	{
	  *cp++ = ',';
	  cp = stpcpy (cp, sponsor);
	}
      if ((mask & CEN_REVISION) != 0)
	{
	  *cp++ = '_';
	  cp = stpcpy (cp, revision);
	}

      *cp++ = '/';
      stpcpy (cp, domain);

      /* Look in list of already loaded domains whether it is already
	 available.  */
      last = NULL;
      for (retval = _nl_loaded_domains; retval != NULL; retval = retval->next)
	if (retval->filename != NULL)
	  {
	    int compare = strcmp (retval->filename, filename);
	    if (compare == 0)
	      /* We found it!  */
	      break;
	    if (compare < 0)
	      {
		/* It's not in the list.  */
		retval = NULL;
		break;
	      }

	    last = retval;
	  }

      if (retval != NULL || do_allocate == 0)
	{
	  free (filename);
	  return retval;
	}
    }

  retval = (struct loaded_domain *) malloc (sizeof (*retval));
  if (retval == NULL)
    return NULL;

  retval->filename = filename;
  retval->decided = 0;

  if (last == NULL)
    {
      retval->next = _nl_loaded_domains;
      _nl_loaded_domains = retval;
    }
  else
    {
      retval->next = last->next;
      last->next = retval;
    }

  entries = 0;
  for (cnt = 126; cnt >= 0; --cnt)
    if (cnt < mask && (cnt & ~mask) == 0
	&& ((cnt & CEN_SPECIFIC) == 0 || (cnt & XPG_SPECIFIC) == 0))
      retval->successor[entries++] = make_entry_rec (dirname, cnt,
						     language, territory,
						     codeset, modifier,
						     special, sponsor,
						     revision, domain, 1);
  retval->successor[entries] = NULL;

  return retval;
}


/* @@ begin of epilog @@ */

/* We don't want libintl.a to depend on any other library.  So we
   avoid the non-standard function stpcpy.  In GNU C Library this
   function is available, though.  Also allow the symbol HAVE_STPCPY
   to be defined.  */
#if !_LIBC && !HAVE_STPCPY
static char *
stpcpy (dest, src)
     char *dest;
     const char *src;
{
  while ((*dest++ = *src++) != '\0')
    /* Do nothing. */ ;
  return dest - 1;
}
#endif
