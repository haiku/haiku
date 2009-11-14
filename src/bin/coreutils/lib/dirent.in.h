/* A GNU-like <dirent.h>.
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

#ifndef _GL_DIRENT_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_DIRENT_H@

#ifndef _GL_DIRENT_H
#define _GL_DIRENT_H

/* The definition of GL_LINK_WARNING is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif

/* Declare overridden functions.  */

#if @REPLACE_FCHDIR@
# define opendir rpl_opendir
extern DIR * opendir (const char *);
# define closedir rpl_closedir
extern int closedir (DIR *);
#endif

/* Declare other POSIX functions.  */

#if @GNULIB_DIRFD@
# if !@HAVE_DECL_DIRFD@ && !defined dirfd
/* Return the file descriptor associated with the given directory stream,
   or -1 if none exists.  */
extern int dirfd (DIR const *dir);
# endif
#elif defined GNULIB_POSIXCHECK
# undef dirfd
# define dirfd(d) \
    (GL_LINK_WARNING ("dirfd is unportable - " \
                      "use gnulib module dirfd for portability"), \
     dirfd (d))
#endif

#if @GNULIB_FDOPENDIR@
# if !@HAVE_FDOPENDIR@
/* Open a directory stream visiting the given directory file
   descriptor.  Return NULL and set errno if fd is not visiting a
   directory.  On success, this function consumes fd (it will be
   implicitly closed either by this function or by a subsequent
   closedir).  */
extern DIR *fdopendir (int fd);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fdopendir
# define fdopendir(f) \
    (GL_LINK_WARNING ("fdopendir is unportable - " \
                      "use gnulib module fdopendir for portability"), \
     fdopendir (f))
#endif

#if @GNULIB_SCANDIR@
/* Scan the directory DIR, calling FILTER on each directory entry.
   Entries for which FILTER returns nonzero are individually malloc'd,
   sorted using qsort with CMP, and collected in a malloc'd array in
   *NAMELIST.  Returns the number of entries selected, or -1 on error.  */
# if !@HAVE_SCANDIR@
extern int scandir (const char *dir, struct dirent ***namelist,
		    int (*filter) (const struct dirent *),
		    int (*cmp) (const struct dirent **, const struct dirent **));
# endif
#elif defined GNULIB_POSIXCHECK
# undef scandir
# define scandir(d,n,f,c) \
    (GL_LINK_WARNING ("scandir is unportable - " \
                      "use gnulib module scandir for portability"), \
     scandir (d, n, f, c))
#endif

#if @GNULIB_ALPHASORT@
/* Compare two 'struct dirent' entries alphabetically.  */
# if !@HAVE_ALPHASORT@
extern int alphasort (const struct dirent **, const struct dirent **);
# endif
#elif defined GNULIB_POSIXCHECK
# undef alphasort
# define alphasort(a,b) \
    (GL_LINK_WARNING ("alphasort is unportable - " \
                      "use gnulib module alphasort for portability"), \
     alphasort (a, b))
#endif

#ifdef __cplusplus
}
#endif


#endif /* _GL_DIRENT_H */
#endif /* _GL_DIRENT_H */
