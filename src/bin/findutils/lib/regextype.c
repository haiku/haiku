
/* regextype.c -- Decode the name of a regular expression syntax into am
                  option name.

   Copyright 2005 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* Written by James Youngman, <jay@gnu.org>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdio.h>

#include "regextype.h"
#include "regex.h"
#include "quote.h"
#include "quotearg.h"
#include "xalloc.h"
#include "error.h"


#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif



struct tagRegexTypeMap
{
  char *name;
  int   option_val;
};

struct tagRegexTypeMap regex_map[] = 
  {
#ifdef FINDUTILS
   { "findutils-default",      RE_SYNTAX_EMACS|RE_DOT_NEWLINE  },
#endif
   { "awk",                    RE_SYNTAX_AWK                   },
   { "egrep",                  RE_SYNTAX_EGREP                 },
#ifndef FINDUTILS
   { "ed",                     RE_SYNTAX_ED                    },
#endif
   { "emacs",                  RE_SYNTAX_EMACS                 },
   { "gnu-awk",                RE_SYNTAX_GNU_AWK               },
   { "grep",                   RE_SYNTAX_GREP                  },
   { "posix-awk",              RE_SYNTAX_POSIX_AWK             },
   { "posix-basic",            RE_SYNTAX_POSIX_BASIC           },
   { "posix-egrep",            RE_SYNTAX_POSIX_EGREP           },
   { "posix-extended",         RE_SYNTAX_POSIX_EXTENDED        },
#ifndef FINDUTILS
   { "posix-minimal-basic",   RE_SYNTAX_POSIX_MINIMAL_BASIC    },
   { "sed",                    RE_SYNTAX_SED                   },
   /*    ,{ "posix-common",   _RE_SYNTAX_POSIX_COMMON   } */
#endif
  };
enum { N_REGEX_MAP_ENTRIES = sizeof(regex_map)/sizeof(regex_map[0]) };

int
get_regex_type(const char *s)
{
  unsigned i;
  size_t msglen;
  char *buf, *p;
  
  msglen = 0u;
  for (i=0u; i<N_REGEX_MAP_ENTRIES; ++i)
    {
      if (0 == strcmp(regex_map[i].name, s))
	return regex_map[i].option_val;
      else
	msglen += strlen(quote(regex_map[i].name)) + 2u;
    }

  /* We didn't find a match for the type of regular expression that the 
   * user indicated they wanted.  Tell them what the options are.
   */
  p = buf = xmalloc(1u + msglen);
  for (i=0u; i<N_REGEX_MAP_ENTRIES; ++i)
    {
      if (i > 0u)
	{
	  strcpy(p, ", ");
	  p += 2;
	}
      p += sprintf(p, "%s", quote(regex_map[i].name));
    }
  
  error(1, 0, _("Unknown regular expression type %s; valid types are %s."),
	quote(s),
	buf);
  /*NOTREACHED*/
  return -1;
}

  
const char *
get_regex_type_name(int ix)
{
  if (ix < N_REGEX_MAP_ENTRIES)
    return regex_map[ix].name;
  else
    return NULL;
}

int
get_regex_type_flags(int ix)
{
  if (ix < N_REGEX_MAP_ENTRIES)
    return regex_map[ix].option_val;
  else
    return -1;
}


int get_regex_type_synonym(int ix)
{
  unsigned i;
  int flags;
  
  if (ix >= N_REGEX_MAP_ENTRIES)
    return -1;
  
  flags = regex_map[ix].option_val;
  for (i=0u; i<ix; ++i)
    {
      if (flags == regex_map[i].option_val)
	{
	  return i;
	}
    }
  return -1;
}
