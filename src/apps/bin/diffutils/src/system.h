/* System dependent declarations.

   Copyright (C) 1988, 1989, 1992, 1993, 1994, 1995, 1998, 2001, 2002
   Free Software Foundation, Inc.

   This file is part of GNU DIFF.

   GNU DIFF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU DIFF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <config.h>

/* Don't bother to support K&R C compilers any more; it's not worth
   the trouble.  These macros prevent some library modules from being
   compiled in K&R C mode.  */
#define PARAMS(Args) Args
#define PROTOTYPES 1

/* Define `__attribute__' and `volatile' first
   so that they're used consistently in all system includes.  */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 6) || __STRICT_ANSI__
# define __attribute__(x)
#endif
#if defined const && !defined volatile
# define volatile
#endif

/* Verify a requirement at compile-time (unlike assert, which is runtime).  */
#define verify(name, assertion) struct name { char a[(assertion) ? 1 : -1]; }


/* Determine whether an integer type is signed, and its bounds.
   This code assumes two's (or one's!) complement with no holes.  */

/* The extra casts work around common compiler bugs,
   e.g. Cray C 5.0.3.0 when t == time_t.  */
#ifndef TYPE_SIGNED
# define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
#endif
#ifndef TYPE_MINIMUM
# define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			       ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) \
			       : (t) 0))
#endif
#ifndef TYPE_MAXIMUM
# define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if STAT_MACROS_BROKEN
# undef S_ISBLK
# undef S_ISCHR
# undef S_ISDIR
# undef S_ISFIFO
# undef S_ISREG
# undef S_ISSOCK
#endif
#ifndef S_ISDIR
# define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
# define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#if !defined S_ISBLK && defined S_IFBLK
# define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#endif
#if !defined S_ISCHR && defined S_IFCHR
# define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif
#if !defined S_ISFIFO && defined S_IFFIFO
# define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFFIFO)
#endif
#if !defined S_ISSOCK && defined S_IFSOCK
# define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#endif
#ifndef S_IXUSR
# define S_IXUSR 0100
#endif
#ifndef S_IXGRP
# define S_IXGRP 0010
#endif
#ifndef S_IXOTH
# define S_IXOTH 0001
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef SEEK_SET
# define SEEK_SET 0
#endif
#ifndef SEEK_CUR
# define SEEK_CUR 1
#endif

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

#if HAVE_TIME_H
# include <time.h>
#else
# include <sys/time.h>
#endif

#if HAVE_FCNTL_H
# include <fcntl.h>
#else
# if HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif

#if !HAVE_DUP2
# define dup2(f, t) (close (t), fcntl (f, F_DUPFD, t))
#endif

#ifndef O_RDONLY
# define O_RDONLY 0
#endif
#ifndef O_RDWR
# define O_RDWR 2
#endif
#ifndef S_IRUSR
# define S_IRUSR 0400
#endif
#ifndef S_IWUSR
# define S_IWUSR 0200
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned int) (stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifndef STAT_BLOCKSIZE
# if HAVE_STRUCT_STAT_ST_BLKSIZE
#  define STAT_BLOCKSIZE(s) ((s).st_blksize)
# else
#  define STAT_BLOCKSIZE(s) (8 * 1024)
# endif
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen ((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) ((dirent)->d_namlen)
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#else
# ifndef getenv
   char *getenv ();
# endif
#endif
#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif
#if !EXIT_FAILURE
# undef EXIT_FAILURE /* Sony NEWS-OS 4.0C defines EXIT_FAILURE to 0.  */
# define EXIT_FAILURE 1
#endif
#define EXIT_TROUBLE 2

#include <limits.h>
#ifndef SSIZE_MAX
# define SSIZE_MAX TYPE_MAXIMUM (ssize_t)
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifndef PTRDIFF_MAX
# define PTRDIFF_MAX TYPE_MAXIMUM (ptrdiff_t)
#endif
#ifndef SIZE_MAX
# define SIZE_MAX TYPE_MAXIMUM (size_t)
#endif
#ifndef UINTMAX_MAX
# define UINTMAX_MAX TYPE_MAXIMUM (uintmax_t)
#endif
#if ! HAVE_STRTOUMAX  && ! defined strtoumax
uintmax_t strtoumax (char const *, char **, int);
#endif

#include <stddef.h>

#if STDC_HEADERS || HAVE_STRING_H
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCHR
#  define memcmp(s1, s2, n) bcmp (s1, s2, n)
#  define memcpy(d, s, n) bcopy (s, d, n)
void *memchr ();
# endif
#endif

#if HAVE_LOCALE_H
# include <locale.h>
#else
# define setlocale(category, locale)
#endif

#include <gettext.h>

#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

#include <ctype.h>

/* CTYPE_DOMAIN (C) is nonzero if the unsigned char C can safely be given
   as an argument to <ctype.h> macros like `isspace'.  */
#if STDC_HEADERS
# define CTYPE_DOMAIN(c) 1
#else
# define CTYPE_DOMAIN(c) ((unsigned int) (c) <= 0177)
#endif
#define ISPRINT(c) (CTYPE_DOMAIN (c) && isprint (c))
#define ISSPACE(c) (CTYPE_DOMAIN (c) && isspace (c))

#if STDC_HEADERS
# define TOLOWER(c) tolower (c)
#else
# ifndef _tolower
#  define _tolower(c) tolower (c)
# endif
# define TOLOWER(c) (CTYPE_DOMAIN (c) && isupper (c) ? _tolower (c) : (c))
#endif

/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX 1003.1-2001 says that only '0' through '9' are digits.
   Prefer ISDIGIT to isdigit unless it's important to use the locale's
   definition of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

#include <errno.h>
#if !STDC_HEADERS
 extern int errno;
#endif

#include <signal.h>
#ifndef SA_RESTART
# ifdef SA_INTERRUPT /* e.g. SunOS 4.1.x */
#  define SA_RESTART SA_INTERRUPT
# else
#  define SA_RESTART 0
# endif
#endif
#if !defined SIGCHLD && defined SIGCLD
# define SIGCHLD SIGCLD
#endif

#undef MIN
#undef MAX
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
# define bool unsigned char
#endif

#if HAVE_VFORK_H
# include <vfork.h>
#endif

#if ! HAVE_WORKING_VFORK
# define vfork fork
#endif

/* Type used for fast comparison of several bytes at a time.  */

#ifndef word
# define word uintmax_t
#endif

/* The integer type of a line number.  Since files are read into main
   memory, ptrdiff_t should be wide enough.  */

typedef ptrdiff_t lin;
#define LIN_MAX PTRDIFF_MAX
verify (lin_is_signed, TYPE_SIGNED (lin));
verify (lin_is_wide_enough, sizeof (ptrdiff_t) <= sizeof (lin));
verify (lin_is_printable_as_long, sizeof (lin) <= sizeof (long));

/* This section contains POSIX-compliant defaults for macros
   that are meant to be overridden by hand in config.h as needed.  */

#ifndef file_name_cmp
# define file_name_cmp strcmp
#endif

#ifndef initialize_main
# define initialize_main(argcp, argvp)
#endif

#ifndef NULL_DEVICE
# define NULL_DEVICE "/dev/null"
#endif

/* Do struct stat *S, *T describe the same special file?  */
#ifndef same_special_file
# if HAVE_ST_RDEV && defined S_ISBLK && defined S_ISCHR
#  define same_special_file(s, t) \
     (((S_ISBLK ((s)->st_mode) && S_ISBLK ((t)->st_mode)) \
       || (S_ISCHR ((s)->st_mode) && S_ISCHR ((t)->st_mode))) \
      && (s)->st_rdev == (t)->st_rdev)
# else
#  define same_special_file(s, t) 0
# endif
#endif

/* Do struct stat *S, *T describe the same file?  Answer -1 if unknown.  */
#ifndef same_file
# define same_file(s, t) \
    ((((s)->st_ino == (t)->st_ino) && ((s)->st_dev == (t)->st_dev)) \
     || same_special_file (s, t))
#endif

/* Do struct stat *S, *T have the same file attributes?

   POSIX says that two files are identical if st_ino and st_dev are
   the same, but many filesystems incorrectly assign the same (device,
   inode) pair to two distinct files, including:

   - GNU/Linux NFS servers that export all local filesystems as a
     single NFS filesystem, if a local device number (st_dev) exceeds
     255, or if a local inode number (st_ino) exceeds 16777215.

   - Network Appliance NFS servers in snapshot directories; see
     Network Appliance bug #195.

   - ClearCase MVFS; see bug id ATRia04618.

   Check whether two files that purport to be the same have the same
   attributes, to work around instances of this common bug.  Do not
   inspect all attributes, only attributes useful in checking for this
   bug.

   It's possible for two distinct files on a buggy filesystem to have
   the same attributes, but it's not worth slowing down all
   implementations (or complicating the configuration) to cater to
   these rare cases in buggy implementations.  */

#ifndef same_file_attributes
# define same_file_attributes(s, t) \
   ((s)->st_mode == (t)->st_mode \
    && (s)->st_nlink == (t)->st_nlink \
    && (s)->st_uid == (t)->st_uid \
    && (s)->st_gid == (t)->st_gid \
    && (s)->st_size == (t)->st_size \
    && (s)->st_mtime == (t)->st_mtime \
    && (s)->st_ctime == (t)->st_ctime)
#endif
