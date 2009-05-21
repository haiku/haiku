/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A GNU-like <arpa/inet.h>.

   Copyright (C) 2005-2006, 2008 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GL_ARPA_INET_H

/* Gnulib's sys/socket.h is responsible for pulling in winsock2.h etc
   under MinGW. */
#include <sys/socket.h>

#if @HAVE_ARPA_INET_H@

# if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
# endif

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_ARPA_INET_H@

#endif

#ifndef _GL_ARPA_INET_H
#define _GL_ARPA_INET_H

/* The definition of GL_LINK_WARNING is copied here.  */

#ifdef __cplusplus
extern "C" {
#endif

#if @GNULIB_INET_NTOP@
# if !@HAVE_DECL_INET_NTOP@
/* Converts an internet address from internal format to a printable,
   presentable format.
   AF is an internet address family, such as AF_INET or AF_INET6.
   SRC points to a 'struct in_addr' (for AF_INET) or 'struct in6_addr'
   (for AF_INET6).
   DST points to a buffer having room for CNT bytes.
   The printable representation of the address (in numeric form, not
   surrounded by [...], no reverse DNS is done) is placed in DST, and
   DST is returned.  If an error occurs, the return value is NULL and
   errno is set.  If CNT bytes are not sufficient to hold the result,
   the return value is NULL and errno is set to ENOSPC.  A good value
   for CNT is 46.

   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/inet_ntop.html>.  */
extern const char *inet_ntop (int af, const void *restrict src,
			      char *restrict dst, socklen_t cnt);
# endif
#elif defined GNULIB_POSIXCHECK
# undef inet_ntop
# define inet_ntop(af,src,dst,cnt) \
    (GL_LINK_WARNING ("inet_ntop is unportable - " \
                      "use gnulib module inet_ntop for portability"), \
     inet_ntop (af, src, dst, cnt))
#endif

#if @GNULIB_INET_PTON@
# if !@HAVE_DECL_INET_PTON@
extern int inet_pton (int af, const char *restrict src, void *restrict dst);
# endif
#elif defined GNULIB_POSIXCHECK
# undef inet_pton
# define inet_pton(af,src,dst) \
  (GL_LINK_WARNING ("inet_pton is unportable - " \
		    "use gnulib module inet_pton for portability"), \
   inet_pton (af, src, dst))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _GL_ARPA_INET_H */
#endif /* _GL_ARPA_INET_H */
