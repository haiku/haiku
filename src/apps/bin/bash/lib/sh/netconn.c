/* netconn.c -- is a particular file descriptor a network connection?. */

/* Copyright (C) 2002 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bash is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA.
*/

#include <config.h>

#include <bashtypes.h>
#ifndef _MINIX
#  include <sys/file.h>
#endif
#include <posixstat.h>
#include <filecntl.h>

#include <errno.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

/* The second and subsequent conditions must match those used to decide
   whether or not to call getpeername() in isnetconn(). */
#if defined (HAVE_SYS_SOCKET_H) && defined (HAVE_GETPEERNAME) && !defined (SVR4_2)
#  include <sys/socket.h>
#endif

/* Is FD a socket or network connection? */
int
isnetconn (fd)
     int fd;
{
#if defined (HAVE_GETPEERNAME) && !defined (SVR4_2) && !defined (__BEOS__)
  int rv;
  socklen_t l;
  struct sockaddr sa;

  l = sizeof(sa);
  rv = getpeername(fd, &sa, &l);
  /* Solaris 2.5 getpeername() returns EINVAL if the fd is not a socket. */
  return ((rv < 0 && (errno == ENOTSOCK || errno == EINVAL)) ? 0 : 1);
#else /* !HAVE_GETPEERNAME || SVR4_2 || __BEOS__ */
#  if defined (SVR4) || defined (SVR4_2)
  /* Sockets on SVR4 and SVR4.2 are character special (streams) devices. */
  struct stat sb;

  if (isatty (fd))
    return (0);
  if (fstat (fd, &sb) < 0)
    return (0);
#    if defined (S_ISFIFO)
  if (S_ISFIFO (sb.st_mode))
    return (0);
#    endif /* S_ISFIFO */
  return (S_ISCHR (sb.st_mode));
#  else /* !SVR4 && !SVR4_2 */
#    if defined (S_ISSOCK) && !defined (__BEOS__)
  struct stat sb;

  if (fstat (fd, &sb) < 0)
    return (0);
  return (S_ISSOCK (sb.st_mode));
#    else /* !S_ISSOCK || __BEOS__ */
  return (0);
#    endif /* !S_ISSOCK || __BEOS__ */
#  endif /* !SVR4 && !SVR4_2 */
#endif /* !HAVE_GETPEERNAME || SVR4_2 || __BEOS__ */
}
