/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Provide a more complete sys/stat header file.
   Copyright (C) 2005-2009 Free Software Foundation, Inc.

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

/* Written by Eric Blake, Paul Eggert, and Jim Meyering.  */

/* This file is supposed to be used on platforms where <sys/stat.h> is
   incomplete.  It is intended to provide definitions and prototypes
   needed by an application.  Start with what the system provides.  */

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if defined __need_system_sys_stat_h
/* Special invocation convention.  */

#@INCLUDE_NEXT@ @NEXT_SYS_STAT_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_SYS_STAT_H

/* Get nlink_t.  */
#include <sys/types.h>

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_SYS_STAT_H@

#ifndef _GL_SYS_STAT_H
#define _GL_SYS_STAT_H

/* The definition of GL_LINK_WARNING is copied here.  */

/* Before doing "#define mkdir rpl_mkdir" below, we need to include all
   headers that may declare mkdir().  */
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
# include <io.h>
#endif

#ifndef S_IFMT
# define S_IFMT 0170000
#endif

#if STAT_MACROS_BROKEN
# undef S_ISBLK
# undef S_ISCHR
# undef S_ISDIR
# undef S_ISFIFO
# undef S_ISLNK
# undef S_ISNAM
# undef S_ISMPB
# undef S_ISMPC
# undef S_ISNWK
# undef S_ISREG
# undef S_ISSOCK
#endif

#ifndef S_ISBLK
# ifdef S_IFBLK
#  define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
# else
#  define S_ISBLK(m) 0
# endif
#endif

#ifndef S_ISCHR
# ifdef S_IFCHR
#  define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
# else
#  define S_ISCHR(m) 0
# endif
#endif

#ifndef S_ISDIR
# ifdef S_IFDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
# else
#  define S_ISDIR(m) 0
# endif
#endif

#ifndef S_ISDOOR /* Solaris 2.5 and up */
# define S_ISDOOR(m) 0
#endif

#ifndef S_ISFIFO
# ifdef S_IFIFO
#  define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
# else
#  define S_ISFIFO(m) 0
# endif
#endif

#ifndef S_ISLNK
# ifdef S_IFLNK
#  define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
# else
#  define S_ISLNK(m) 0
# endif
#endif

#ifndef S_ISMPB /* V7 */
# ifdef S_IFMPB
#  define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
#  define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
# else
#  define S_ISMPB(m) 0
#  define S_ISMPC(m) 0
# endif
#endif

#ifndef S_ISNAM /* Xenix */
# ifdef S_IFNAM
#  define S_ISNAM(m) (((m) & S_IFMT) == S_IFNAM)
# else
#  define S_ISNAM(m) 0
# endif
#endif

#ifndef S_ISNWK /* HP/UX */
# ifdef S_IFNWK
#  define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
# else
#  define S_ISNWK(m) 0
# endif
#endif

#ifndef S_ISPORT /* Solaris 10 and up */
# define S_ISPORT(m) 0
#endif

#ifndef S_ISREG
# ifdef S_IFREG
#  define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
# else
#  define S_ISREG(m) 0
# endif
#endif

#ifndef S_ISSOCK
# ifdef S_IFSOCK
#  define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
# else
#  define S_ISSOCK(m) 0
# endif
#endif


#ifndef S_TYPEISMQ
# define S_TYPEISMQ(p) 0
#endif

#ifndef S_TYPEISTMO
# define S_TYPEISTMO(p) 0
#endif


#ifndef S_TYPEISSEM
# ifdef S_INSEM
#  define S_TYPEISSEM(p) (S_ISNAM ((p)->st_mode) && (p)->st_rdev == S_INSEM)
# else
#  define S_TYPEISSEM(p) 0
# endif
#endif

#ifndef S_TYPEISSHM
# ifdef S_INSHD
#  define S_TYPEISSHM(p) (S_ISNAM ((p)->st_mode) && (p)->st_rdev == S_INSHD)
# else
#  define S_TYPEISSHM(p) 0
# endif
#endif

/* high performance ("contiguous data") */
#ifndef S_ISCTG
# define S_ISCTG(p) 0
#endif

/* Cray DMF (data migration facility): off line, with data  */
#ifndef S_ISOFD
# define S_ISOFD(p) 0
#endif

/* Cray DMF (data migration facility): off line, with no data  */
#ifndef S_ISOFL
# define S_ISOFL(p) 0
#endif

/* 4.4BSD whiteout */
#ifndef S_ISWHT
# define S_ISWHT(m) 0
#endif

/* If any of the following are undefined,
   define them to their de facto standard values.  */
#if !S_ISUID
# define S_ISUID 04000
#endif
#if !S_ISGID
# define S_ISGID 02000
#endif

/* S_ISVTX is a common extension to POSIX.  */
#ifndef S_ISVTX
# define S_ISVTX 01000
#endif

#if !S_IRUSR && S_IREAD
# define S_IRUSR S_IREAD
#endif
#if !S_IRUSR
# define S_IRUSR 00400
#endif
#if !S_IRGRP
# define S_IRGRP (S_IRUSR >> 3)
#endif
#if !S_IROTH
# define S_IROTH (S_IRUSR >> 6)
#endif

#if !S_IWUSR && S_IWRITE
# define S_IWUSR S_IWRITE
#endif
#if !S_IWUSR
# define S_IWUSR 00200
#endif
#if !S_IWGRP
# define S_IWGRP (S_IWUSR >> 3)
#endif
#if !S_IWOTH
# define S_IWOTH (S_IWUSR >> 6)
#endif

#if !S_IXUSR && S_IEXEC
# define S_IXUSR S_IEXEC
#endif
#if !S_IXUSR
# define S_IXUSR 00100
#endif
#if !S_IXGRP
# define S_IXGRP (S_IXUSR >> 3)
#endif
#if !S_IXOTH
# define S_IXOTH (S_IXUSR >> 6)
#endif

#if !S_IRWXU
# define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif
#if !S_IRWXG
# define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#endif
#if !S_IRWXO
# define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif

/* S_IXUGO is a common extension to POSIX.  */
#if !S_IXUGO
# define S_IXUGO (S_IXUSR | S_IXGRP | S_IXOTH)
#endif

#ifndef S_IRWXUGO
# define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif


#ifdef __cplusplus
extern "C" {
#endif


#if @GNULIB_LSTAT@
# if ! @HAVE_LSTAT@
/* mingw does not support symlinks, therefore it does not have lstat.  But
   without links, stat does just fine.  */
#  define lstat stat
# elif @REPLACE_LSTAT@
#  undef lstat
#  define lstat rpl_lstat
extern int rpl_lstat (const char *name, struct stat *buf);
# endif
#elif defined GNULIB_POSIXCHECK
# undef lstat
# define lstat(p,b)							\
  (GL_LINK_WARNING ("lstat is unportable - "				\
		    "use gnulib module lstat for portability"),		\
   lstat (p, b))
#endif


#if @GNULIB_FCHMODAT@
# if !@HAVE_FCHMODAT@
extern int fchmodat (int fd, char const *file, mode_t mode, int flag);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fchmodat
# define fchmodat(d,n,m,f)                         \
    (GL_LINK_WARNING ("fchmodat is not portable - " \
                      "use gnulib module openat for portability"), \
     fchmodat (d, n, m, f))
#endif


#if @GNULIB_FSTATAT@
# if @REPLACE_FSTATAT@
#  undef fstatat
#  define fstatat rpl_fstatat
# endif
# if !@HAVE_FSTATAT@ || @REPLACE_FSTATAT@
extern int fstatat (int fd, char const *name, struct stat *st, int flags);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fstatat
# define fstatat(d,n,s,f)                         \
    (GL_LINK_WARNING ("fstatat is not portable - " \
                      "use gnulib module openat for portability"), \
     fstatat (d, n, s, f))
#endif


#if @GNULIB_MKDIRAT@
# if !@HAVE_MKDIRAT@
extern int mkdirat (int fd, char const *file, mode_t mode);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mkdirat
# define mkdirat(d,n,m)                         \
    (GL_LINK_WARNING ("mkdirat is not portable - " \
                      "use gnulib module openat for portability"), \
     mkdirat (d, n, m))
#endif

#if @GNULIB_MKFIFOAT@
# if !@HAVE_MKFIFOAT@
int mkfifoat (int fd, char const *file, mode_t mode);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mkfifoat
# define mkfifoat(d,n,m)				     \
    (GL_LINK_WARNING ("mkfifoat is not portable - " \
                      "use gnulib module mkfifoat for portability"), \
     mkfifoat (d, n, m))
#endif

#if @GNULIB_MKNODAT@
# if !@HAVE_MKNODAT@
int mknodat (int fd, char const *file, mode_t mode, dev_t dev);
# endif
#elif defined GNULIB_POSIXCHECK
# undef mknodat
# define mknodat(f,n,m,d)			     \
    (GL_LINK_WARNING ("mknodat is not portable - " \
                      "use gnulib module mkfifoat for portability"), \
     mknodat (f, n, m, d))
#endif

#if @REPLACE_FCHDIR@
# define fstat rpl_fstat
extern int fstat (int fd, struct stat *buf);
#endif

#if @REPLACE_MKDIR@
# undef mkdir
# define mkdir rpl_mkdir
extern int mkdir (char const *name, mode_t mode);
#else
/* mingw's _mkdir() function has 1 argument, but we pass 2 arguments.
   Additionally, it declares _mkdir (and depending on compile flags, an
   alias mkdir), only in the nonstandard <io.h>, which is included above.  */
# if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

static inline int
rpl_mkdir (char const *name, mode_t mode)
{
  return _mkdir (name);
}

#  define mkdir rpl_mkdir
# endif
#endif


/* Declare BSD extensions.  */

#if @GNULIB_LCHMOD@
/* Change the mode of FILENAME to MODE, without dereferencing it if FILENAME
   denotes a symbolic link.  */
# if !@HAVE_LCHMOD@
/* The lchmod replacement follows symbolic links.  Callers should take
   this into account; lchmod should be applied only to arguments that
   are known to not be symbolic links.  On hosts that lack lchmod,
   this can lead to race conditions between the check and the
   invocation of lchmod, but we know of no workarounds that are
   reliable in general.  You might try requesting support for lchmod
   from your operating system supplier.  */
#  define lchmod chmod
# endif
# if 0 /* assume already declared */
extern int lchmod (const char *filename, mode_t mode);
# endif
#elif defined GNULIB_POSIXCHECK
# undef lchmod
# define lchmod(f,m) \
    (GL_LINK_WARNING ("lchmod is unportable - " \
                      "use gnulib module lchmod for portability"), \
     lchmod (f, m))
#endif


#ifdef __cplusplus
}
#endif


#endif /* _GL_SYS_STAT_H */
#endif /* _GL_SYS_STAT_H */
#endif
