/* modetype.h -- file type bits definitions for POSIX systems
   Requires sys/types.h sys/stat.h.
   Copyright (C) 1990, 2001, 2003, 2004 Free Software Foundation, Inc.

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

/* POSIX.1 doesn't mention the S_IFMT bits; instead, it uses S_IStype
   test macros.  To make storing file types more convenient, define
   them; the values don't need to correspond to what the kernel uses,
   because of the way we use them. */
#ifndef INC_MODETYPE_H
#define INC_MODETYPE_H 1

#ifndef S_IFMT			/* Doesn't have traditional Unix macros. */
#define S_IFBLK 1
#define S_IFCHR 2
#define S_IFDIR 4
#define S_IFREG 8
#ifdef S_ISLNK
#define S_IFLNK 16
#endif
#ifdef S_ISFIFO
#define S_IFIFO 32
#endif
#ifdef S_ISSOCK
#define S_IFSOCK 64
#endif
#ifdef S_ISDOOR
#define S_IFDOOR 128
#endif
#endif /* !S_IFMT */

#ifdef STAT_MACROS_BROKEN
#undef S_ISBLK
#undef S_ISCHR
#undef S_ISDIR
#undef S_ISREG
#undef S_ISFIFO
#undef S_ISLNK
#undef S_ISSOCK
#undef S_ISDOOR
#undef S_ISMPB
#undef S_ISMPC
#undef S_ISNWK
#endif

/* Do the reverse: define the POSIX.1 macros for traditional Unix systems
   that don't have them.  */
#if !defined(S_ISBLK) && defined(S_IFBLK)
#define	S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
#define	S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define	S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
#define	S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
#define	S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
#define	S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
#define	S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISDOOR) && defined(S_IFDOOR)
#define	S_ISDOOR(m) (((m) & S_IFMT) == S_IFDOOR)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
#define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
#define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
#endif
#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
#define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
#endif

#endif
