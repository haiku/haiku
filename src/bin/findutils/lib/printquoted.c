/* printquoted.c -- print a specified string with any necessary quoting.

   Copyright (C) 1990, 1991, 1992, 1993, 1994, 2000, 
                 2003, 2004, 2005 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

/* Get mbstate_t, mbrtowc(), mbsinit(), wcwidth().  */
#if HAVE_WCHAR_H
# include <wchar.h>
#endif



#include "xalloc.h"
#include "printquoted.h"


/*
 * Print S according to the format FORMAT, but if the destination is a tty, 
 * convert any potentially-dangerous characters.  The logic in this function 
 * was taken from ls.c in coreutils (at Sun Jun  5 20:42:51 2005 UTC).
 */
void
print_quoted (FILE *fp,
	      const struct quoting_options *qopts,
	      bool dest_is_tty,
	      const char *format,
	      const char *s)
{
  if (dest_is_tty)
    {
      char smallbuf[BUFSIZ];
      size_t len = quotearg_buffer (smallbuf, sizeof smallbuf, s, -1, qopts);
      char *buf;
      if (len < sizeof smallbuf)
	buf = smallbuf;
      else
	{
	  /* The original coreutils code uses alloca(), but I don't
	   * want to take on the anguish of introducing alloca() to
	   * 'find'.
	   */
	  buf = xmalloc (len + 1);
	  quotearg_buffer (buf, len + 1, s, -1, qopts);
	}

      /* Replace any remaining funny characters with '?'. */
      len = qmark_chars(buf, len);
      
      fprintf(fp, format, buf);	/* Print the quoted version */
      if (buf != smallbuf)
	{
	  free(buf);
	  buf = NULL;
	}
    }
  else
    {
      /* no need to quote things. */
      fprintf(fp, format, s);
    }
  
}

