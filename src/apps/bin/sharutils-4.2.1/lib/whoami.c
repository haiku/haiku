/* Find out who I am & where I am.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "system.h"

/* Size of buffers holding strings.  */
#define STRING_BUFFER_SIZE 64

#include <pwd.h>

/* On Ultrix 2.x, <utsname.h> uses SYS_NMLEN, which is only defined in
   <limits.h>.  Sigh!  */

#ifdef HAVE_UNAME
# ifdef HAVE_LIMITS_H
#  include <limits.h>
# endif
# include <sys/utsname.h>
# include <time.h>
#else
# include <sys/time.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
#endif

/*----------------.
| Get user name.  |
`----------------*/

static const char *
get_username ()
{
  struct passwd *passwd;

  passwd = getpwuid (getuid ());
  endpwent ();
  if (passwd == NULL)
    return "???";
  return passwd->pw_name;
}

/*------------------------------------------------------.
| Do uname, gethostname, or read file (/etc/systemid).  |
`------------------------------------------------------*/

static const char *
get_hostname ()
{
#if HAVE_UNAME

  static struct utsname hostname_buffer;

  uname (&hostname_buffer);
  return hostname_buffer.nodename;

#else
# if HAVE_ETC_SYSTEMID

  FILE *fpsid = fopen ("/etc/systemid", "r");
  static char buffer[STRING_BUFFER_SIZE];

  if (!fpsid)
    return "???";
  fgets (buffer, sizeof (buffer), fpsid);
  fclose (fpsid);
  buffer[strlen (buffer) - 1] = 0;
  return buffer;

# else

  static char hostname_buffer[STRING_BUFFER_SIZE];

  gethostname (hostname_buffer, sizeof (hostname_buffer));
  return hostname_buffer;

# endif
#endif
}

/*------------------------------------------------------------------.
| Fill BUFFER with a string representing the username and hostname  |
| separated by an `@' sign, and return a pointer to the constructed |
| string.  If BUFFER is NULL, use an internal buffer instead.	    |
`------------------------------------------------------------------*/

char *
get_submitter (buffer)
     char *buffer;
{
  static char static_buffer[STRING_BUFFER_SIZE];

  if (!buffer)
    buffer = static_buffer;
  strcpy (buffer, get_username ());
  strcat (buffer, "@");
  return strcat (buffer, get_hostname ());
}
