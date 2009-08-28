/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Substitute for <sys/select.h>.
   Copyright (C) 2007-2009 Free Software Foundation, Inc.

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

# if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
# endif

/* On OSF/1, <sys/types.h> and <sys/time.h> include <sys/select.h>.
   Simply delegate to the system's header in this case.  */
#if @HAVE_SYS_SELECT_H@ && defined __osf__ && (defined _SYS_TYPES_H_ && !defined _GL_SYS_SELECT_H_REDIRECT_FROM_SYS_TYPES_H) && defined _OSF_SOURCE

# define _GL_SYS_SELECT_H_REDIRECT_FROM_SYS_TYPES_H
# @INCLUDE_NEXT@ @NEXT_SYS_SELECT_H@

#elif @HAVE_SYS_SELECT_H@ && defined __osf__ && (defined _SYS_TIME_H_ && !defined _GL_SYS_SELECT_H_REDIRECT_FROM_SYS_TIME_H) && defined _OSF_SOURCE

# define _GL_SYS_SELECT_H_REDIRECT_FROM_SYS_TIME_H
# @INCLUDE_NEXT@ @NEXT_SYS_SELECT_H@

#else

#ifndef _GL_SYS_SELECT_H

#if @HAVE_SYS_SELECT_H@

/* On many platforms, <sys/select.h> assumes prior inclusion of
   <sys/types.h>.  */
# include <sys/types.h>

/* On OSF/1 4.0, <sys/select.h> provides only a forward declaration
   of 'struct timeval', and no definition of this type.  */
# include <sys/time.h>

/* On Solaris 10, <sys/select.h> provides an FD_ZERO implementation
   that relies on memset(), but without including <string.h>.  */
# include <string.h>

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_SYS_SELECT_H@

#endif

#ifndef _GL_SYS_SELECT_H
#define _GL_SYS_SELECT_H

#if !@HAVE_SYS_SELECT_H@

/* A platform that lacks <sys/select.h>.  */

# include <sys/socket.h>

/* The definition of GL_LINK_WARNING is copied here.  */

# ifdef __cplusplus
extern "C" {
# endif

# if @GNULIB_SELECT@
#  if @HAVE_WINSOCK2_H@ || @REPLACE_SELECT@
#   undef select
#   define select rpl_select
extern int rpl_select (int, fd_set *, fd_set *, fd_set *, struct timeval *);
#  endif
# elif @HAVE_WINSOCK2_H@
#  undef select
#  define select select_used_without_requesting_gnulib_module_select
# elif defined GNULIB_POSIXCHECK
#  undef select
#  define select(n,r,w,e,t) \
     (GL_LINK_WARNING ("select is not always POSIX compliant - " \
                       "use gnulib module select for portability"), \
      select (n, r, w, e, t))
# endif

# ifdef __cplusplus
}
# endif

#endif

#endif /* _GL_SYS_SELECT_H */
#endif /* _GL_SYS_SELECT_H */
#endif /* OSF/1 */
