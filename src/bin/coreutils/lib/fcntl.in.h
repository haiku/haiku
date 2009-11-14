/* Like <fcntl.h>, but with non-working flags defined to 0.

   Copyright (C) 2006-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* written by Paul Eggert */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if defined __need_system_fcntl_h
/* Special invocation convention.  */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#@INCLUDE_NEXT@ @NEXT_FCNTL_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_FCNTL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_FCNTL_H@

#ifndef _GL_FCNTL_H
#define _GL_FCNTL_H


/* The definition of GL_LINK_WARNING is copied here.  */


/* Declare overridden functions.  */

#ifdef __cplusplus
extern "C" {
#endif

#if @GNULIB_OPEN@
# if @REPLACE_OPEN@
#  undef open
#  define open rpl_open
extern int open (const char *filename, int flags, ...);
# endif
#endif

#if @GNULIB_OPENAT@
# if !@HAVE_OPENAT@
#  undef openat
#  define openat rpl_openat
int openat (int fd, char const *file, int flags, /* mode_t mode */ ...);
# endif
#elif defined GNULIB_POSIXCHECK
# undef openat
# define openat(f,u,g) \
    (GL_LINK_WARNING ("openat is not portable - " \
                      "use gnulib module openat for portability"), \
     openat)
#endif

#ifdef __cplusplus
}
#endif

/* Fix up the FD_* macros.  */

#ifndef FD_CLOEXEC
# define FD_CLOEXEC 1
#endif

/* Fix up the O_* macros.  */

#if !defined O_DIRECT && defined O_DIRECTIO
/* Tru64 spells it `O_DIRECTIO'.  */
# define O_DIRECT O_DIRECTIO
#endif

#if !defined O_CLOEXEC && defined O_NOINHERIT
/* Mingw spells it `O_NOINHERIT'.  Intentionally leave it
   undefined if not available.  */
# define O_CLOEXEC O_NOINHERIT
#endif

#ifndef O_DIRECT
# define O_DIRECT 0
#endif

#ifndef O_DIRECTORY
# define O_DIRECTORY 0
#endif

#ifndef O_DSYNC
# define O_DSYNC 0
#endif

#ifndef O_NDELAY
# define O_NDELAY 0
#endif

#ifndef O_NOATIME
# define O_NOATIME 0
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK O_NDELAY
#endif

#ifndef O_NOCTTY
# define O_NOCTTY 0
#endif

#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

#ifndef O_NOLINKS
# define O_NOLINKS 0
#endif

#ifndef O_RSYNC
# define O_RSYNC 0
#endif

#ifndef O_SYNC
# define O_SYNC 0
#endif

#ifndef O_TTY_INIT
# define O_TTY_INIT 0
#endif

/* For systems that distinguish between text and binary I/O.
   O_BINARY is usually declared in fcntl.h  */
#if !defined O_BINARY && defined _O_BINARY
  /* For MSC-compatible compilers.  */
# define O_BINARY _O_BINARY
# define O_TEXT _O_TEXT
#endif

#if defined __BEOS__ || defined __HAIKU__
  /* BeOS 5 and Haiku have O_BINARY and O_TEXT, but they have no effect.  */
# undef O_BINARY
# undef O_TEXT
#endif

#ifndef O_BINARY
# define O_BINARY 0
# define O_TEXT 0
#endif

/* Fix up the AT_* macros.  */

/* Work around a bug in Solaris 9 and 10: AT_FDCWD is positive.  Its
   value exceeds INT_MAX, so its use as an int doesn't conform to the
   C standard, and GCC and Sun C complain in some cases.  If the bug
   is present, undef AT_FDCWD here, so it can be redefined below.  */
#if 0 < AT_FDCWD && AT_FDCWD == 0xffd19553
# undef AT_FDCWD
#endif

/* Use the same bit pattern as Solaris 9, but with the proper
   signedness.  The bit pattern is important, in case this actually is
   Solaris with the above workaround.  */
#ifndef AT_FDCWD
# define AT_FDCWD (-3041965)
#endif

/* Use the same values as Solaris 9.  This shouldn't matter, but
   there's no real reason to differ.  */
#ifndef AT_SYMLINK_NOFOLLOW
# define AT_SYMLINK_NOFOLLOW 4096
#endif

#ifndef AT_REMOVEDIR
# define AT_REMOVEDIR 1
#endif

/* Solaris 9 lacks these two, so just pick unique values.  */
#ifndef AT_SYMLINK_FOLLOW
# define AT_SYMLINK_FOLLOW 2
#endif

#ifndef AT_EACCESS
# define AT_EACCESS 4
#endif


#endif /* _GL_FCNTL_H */
#endif /* _GL_FCNTL_H */
#endif
