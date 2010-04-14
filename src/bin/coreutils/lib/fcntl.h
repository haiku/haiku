/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* Like <fcntl.h>, but with non-working flags defined to 0.

   Copyright (C) 2006-2010 Free Software Foundation, Inc.

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
#pragma GCC system_header
#endif

#if defined __need_system_fcntl_h
/* Special invocation convention.  */

#include <sys/types.h>
#ifndef __GLIBC__ /* Avoid namespace pollution on glibc systems.  */
# include <sys/stat.h>
# include <unistd.h>
#endif
#include_next <fcntl.h>

#else
/* Normal invocation convention.  */

#ifndef _GL_FCNTL_H

#include <sys/types.h>
#ifndef __GLIBC__ /* Avoid namespace pollution on glibc systems.  */
# include <sys/stat.h>
# include <unistd.h>
#endif
/* The include_next requires a split double-inclusion guard.  */
#include_next <fcntl.h>

#ifndef _GL_FCNTL_H
#define _GL_FCNTL_H


/* The definition of GL_LINK_WARNING is copied here.  */
/* GL_LINK_WARNING("literal string") arranges to emit the literal string as
   a linker warning on most glibc systems.
   We use a linker warning rather than a preprocessor warning, because
   #warning cannot be used inside macros.  */
#ifndef GL_LINK_WARNING
  /* This works on platforms with GNU ld and ELF object format.
     Testing __GLIBC__ is sufficient for asserting that GNU ld is in use.
     Testing __ELF__ guarantees the ELF object format.
     Testing __GNUC__ is necessary for the compound expression syntax.  */
# if defined __GLIBC__ && defined __ELF__ && defined __GNUC__
#  define GL_LINK_WARNING(message) \
     GL_LINK_WARNING1 (__FILE__, __LINE__, message)
#  define GL_LINK_WARNING1(file, line, message) \
     GL_LINK_WARNING2 (file, line, message)  /* macroexpand file and line */
#  define GL_LINK_WARNING2(file, line, message) \
     GL_LINK_WARNING3 (file ":" #line ": warning: " message)
#  define GL_LINK_WARNING3(message) \
     ({ static const char warning[sizeof (message)]             \
          __attribute__ ((__unused__,                           \
                          __section__ (".gnu.warning"),         \
                          __aligned__ (1)))                     \
          = message "\n";                                       \
        (void)0;                                                \
     })
# else
#  define GL_LINK_WARNING(message) ((void) 0)
# endif
#endif

/* The definition of _GL_ARG_NONNULL is copied here.  */
/* _GL_ARG_NONNULL((n,...,m)) tells the compiler and static analyzer tools
   that the values passed as arguments n, ..., m must be non-NULL pointers.
   n = 1 stands for the first argument, n = 2 for the second argument etc.  */
#ifndef _GL_ARG_NONNULL
# if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ > 3
#  define _GL_ARG_NONNULL(params) __attribute__ ((__nonnull__ params))
# else
#  define _GL_ARG_NONNULL(params)
# endif
#endif


/* Declare overridden functions.  */

#ifdef __cplusplus
extern "C" {
#endif

#if 1
# if 1
#  undef fcntl
#  define fcntl rpl_fcntl
# endif
# if !1 || 1
extern int fcntl (int fd, int action, ...);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fcntl
# define fcntl \
    (GL_LINK_WARNING ("fcntl is not always POSIX compliant - " \
                      "use gnulib module fcntl for portability"), \
     fcntl)
#endif

#if 1
# if 1
#  undef open
#  define open rpl_open
extern int open (const char *filename, int flags, ...) _GL_ARG_NONNULL ((1));
# endif
#elif defined GNULIB_POSIXCHECK
# undef open
# define open \
    (GL_LINK_WARNING ("open is not always POSIX compliant - " \
                      "use gnulib module open for portability"), \
     open)
#endif

#if 1
# if 1
#  undef openat
#  define openat rpl_openat
# endif
# if !1 || 1
extern int openat (int fd, char const *file, int flags, /* mode_t mode */ ...)
     _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef openat
# define openat \
    (GL_LINK_WARNING ("openat is not portable - " \
                      "use gnulib module openat for portability"), \
     openat)
#endif

#ifdef __cplusplus
}
#endif

/* Fix up the FD_* macros, only known to be missing on mingw.  */

#ifndef FD_CLOEXEC
# define FD_CLOEXEC 1
#endif

/* Fix up the supported F_* macros.  Intentionally leave other F_*
   macros undefined.  Only known to be missing on mingw.  */

#ifndef F_DUPFD_CLOEXEC
# define F_DUPFD_CLOEXEC 0x40000000
/* Witness variable: 1 if gnulib defined F_DUPFD_CLOEXEC, 0 otherwise.  */
# define GNULIB_defined_F_DUPFD_CLOEXEC 1
#else
# define GNULIB_defined_F_DUPFD_CLOEXEC 0
#endif

#ifndef F_DUPFD
# define F_DUPFD 1
#endif

#ifndef F_GETFD
# define F_GETFD 2
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
