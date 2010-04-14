/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Substitute for <sys/utsname.h>.
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

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

#ifndef _GL_SYS_UTSNAME_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if @HAVE_SYS_UTSNAME_H@
# @INCLUDE_NEXT@ @NEXT_SYS_UTSNAME_H@
#endif

#define _GL_SYS_UTSNAME_H

/* The definition of GL_LINK_WARNING is copied here.  */

/* The definition of _GL_ARG_NONNULL is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif

#if !@HAVE_STRUCT_UTSNAME@
/* Length of the entries in 'struct utsname' is 256.  */
# define _UTSNAME_LENGTH 256

# ifndef _UTSNAME_NODENAME_LENGTH
#  define _UTSNAME_NODENAME_LENGTH _UTSNAME_LENGTH
# endif
# ifndef _UTSNAME_SYSNAME_LENGTH
#  define _UTSNAME_SYSNAME_LENGTH _UTSNAME_LENGTH
# endif
# ifndef _UTSNAME_RELEASE_LENGTH
#  define _UTSNAME_RELEASE_LENGTH _UTSNAME_LENGTH
# endif
# ifndef _UTSNAME_VERSION_LENGTH
#  define _UTSNAME_VERSION_LENGTH _UTSNAME_LENGTH
# endif
# ifndef _UTSNAME_MACHINE_LENGTH
#  define _UTSNAME_MACHINE_LENGTH _UTSNAME_LENGTH
# endif

/* Structure describing the system and machine.  */
struct utsname
  {
    /* Name of this node on the network.  */
    char nodename[_UTSNAME_NODENAME_LENGTH];

    /* Name of the implementation of the operating system.  */
    char sysname[_UTSNAME_SYSNAME_LENGTH];
    /* Current release level of this implementation.  */
    char release[_UTSNAME_RELEASE_LENGTH];
    /* Current version level of this release.  */
    char version[_UTSNAME_VERSION_LENGTH];

    /* Name of the hardware type the system is running on.  */
    char machine[_UTSNAME_MACHINE_LENGTH];
  };
#endif /* !@HAVE_STRUCT_UTSNAME@ */


#if @GNULIB_UNAME@
# if !@HAVE_UNAME@
extern int uname (struct utsname *buf) _GL_ARG_NONNULL ((1));
# endif
#elif defined GNULIB_POSIXCHECK
# undef uname
# define uname(b) \
    (GL_LINK_WARNING ("uname is unportable - " \
                      "use gnulib module uname for portability"), \
     uname (b))
#endif


#ifdef __cplusplus
}
#endif


#endif /* _GL_SYS_UTSNAME_H */
