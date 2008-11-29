/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* A GNU-like <stdio.h>.

   Copyright (C) 2004, 2007 Free Software Foundation, Inc.

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

#if defined __need_FILE || defined __need___FILE
/* Special invocation convention inside glibc header files.  */

#include_next <stdio.h>

#else
/* Normal invocation convention.  */

#ifndef _GL_STDIO_H

/* The include_next requires a split double-inclusion guard.  */
#include_next <stdio.h>

#ifndef _GL_STDIO_H
#define _GL_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#if (1 && 0) \
  || (1 && 0) \
  || (1 && !0) \
  || (1 && (!0 || 0))
/* Get off_t and ssize_t.  */
# include <sys/types.h>
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(Spec) /* empty */
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif


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
     ({ static const char warning[sizeof (message)]		\
          __attribute__ ((__unused__,				\
                          __section__ (".gnu.warning"),		\
                          __aligned__ (1)))			\
          = message "\n";					\
        (void)0;						\
     })
# else
#  define GL_LINK_WARNING(message) ((void) 0)
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


#if 0
# if 0
#  define fprintf rpl_fprintf
extern int fprintf (FILE *fp, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef fprintf
# define fprintf \
    (GL_LINK_WARNING ("fprintf is not always POSIX compliant - " \
                      "use gnulib module fprintf-posix for portable " \
                      "POSIX compliance"), \
     fprintf)
#endif

#if 0
# if 0
#  define vfprintf rpl_vfprintf
extern int vfprintf (FILE *fp, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vfprintf
# define vfprintf(s,f,a) \
    (GL_LINK_WARNING ("vfprintf is not always POSIX compliant - " \
                      "use gnulib module vfprintf-posix for portable " \
                      "POSIX compliance"), \
     vfprintf (s, f, a))
#endif

#if 0
# if 0
/* Don't break __attribute__((format(printf,M,N))).  */
#  define printf __printf__
extern int printf (const char *format, ...)
       __attribute__ ((__format__ (__printf__, 1, 2)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef printf
# define printf \
    (GL_LINK_WARNING ("printf is not always POSIX compliant - " \
                      "use gnulib module printf-posix for portable " \
                      "POSIX compliance"), \
     printf)
/* Don't break __attribute__((format(printf,M,N))).  */
# define format(kind,m,n) format (__##kind##__, m, n)
# define __format__(kind,m,n) __format__ (__##kind##__, m, n)
# define ____printf____ __printf__
# define ____scanf____ __scanf__
# define ____strftime____ __strftime__
# define ____strfmon____ __strfmon__
#endif

#if 0
# if 0
#  define vprintf rpl_vprintf
extern int vprintf (const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 1, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vprintf
# define vprintf(f,a) \
    (GL_LINK_WARNING ("vprintf is not always POSIX compliant - " \
                      "use gnulib module vprintf-posix for portable " \
                      "POSIX compliance"), \
     vprintf (f, a))
#endif

#if 0
# if 0
#  define snprintf rpl_snprintf
# endif
# if 0 || !1
extern int snprintf (char *str, size_t size, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 3, 4)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef snprintf
# define snprintf \
    (GL_LINK_WARNING ("snprintf is unportable - " \
                      "use gnulib module snprintf for portability"), \
     snprintf)
#endif

#if 0
# if 0
#  define vsnprintf rpl_vsnprintf
# endif
# if 0 || !1
extern int vsnprintf (char *str, size_t size, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 3, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vsnprintf
# define vsnprintf(b,s,f,a) \
    (GL_LINK_WARNING ("vsnprintf is unportable - " \
                      "use gnulib module vsnprintf for portability"), \
     vsnprintf (b, s, f, a))
#endif

#if 0
# if 0
#  define sprintf rpl_sprintf
extern int sprintf (char *str, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef sprintf
# define sprintf \
    (GL_LINK_WARNING ("sprintf is not always POSIX compliant - " \
                      "use gnulib module sprintf-posix for portable " \
                      "POSIX compliance"), \
     sprintf)
#endif

#if 0
# if 0
#  define vsprintf rpl_vsprintf
extern int vsprintf (char *str, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vsprintf
# define vsprintf(b,f,a) \
    (GL_LINK_WARNING ("vsprintf is not always POSIX compliant - " \
                      "use gnulib module vsprintf-posix for portable " \
                      "POSIX compliance"), \
     vsprintf (b, f, a))
#endif

#if 0
# if 0
#  define asprintf rpl_asprintf
#  define vasprintf rpl_vasprintf
# endif
# if 0 || !1
  /* Write formatted output to a string dynamically allocated with malloc().
     If the memory allocation succeeds, store the address of the string in
     *RESULT and return the number of resulting bytes, excluding the trailing
     NUL.  Upon memory allocation error, or some other error, return -1.  */
  extern int asprintf (char **result, const char *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
  extern int vasprintf (char **result, const char *format, va_list args)
    __attribute__ ((__format__ (__printf__, 2, 0)));
# endif
#endif

#if 0
# if 0
#  define fopen rpl_fopen
extern FILE * fopen (const char *filename, const char *mode);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fopen
# define fopen(f,m) \
   (GL_LINK_WARNING ("fopen on Win32 platforms is not POSIX compatible - " \
                     "use gnulib module fopen for portability"), \
    fopen (f, m))
#endif

#if 0
# if 0
#  define freopen rpl_freopen
extern FILE * freopen (const char *filename, const char *mode, FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef freopen
# define freopen(f,m,s) \
   (GL_LINK_WARNING ("freopen on Win32 platforms is not POSIX compatible - " \
                     "use gnulib module freopen for portability"), \
    freopen (f, m, s))
#endif

#if 1
# if 0
/* Provide fseek, fseeko functions that are aware of a preceding
   fflush(), and which detect pipes.  */
#  define fseeko rpl_fseeko
extern int fseeko (FILE *fp, off_t offset, int whence);
#  define fseek(fp, offset, whence) fseeko (fp, (off_t)(offset), whence)
# endif
#elif defined GNULIB_POSIXCHECK
# undef fseeko
# define fseeko(f,o,w) \
   (GL_LINK_WARNING ("fseeko is unportable - " \
                     "use gnulib module fseeko for portability"), \
    fseeko (f, o, w))
#endif

#if 0 && 0
extern int rpl_fseek (FILE *fp, long offset, int whence);
# undef fseek
# if defined GNULIB_POSIXCHECK
#  define fseek(f,o,w) \
     (GL_LINK_WARNING ("fseek cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use fseeko function for handling of large files"), \
      rpl_fseek (f, o, w))
# else
#  define fseek rpl_fseek
# endif
#elif defined GNULIB_POSIXCHECK
# ifndef fseek
#  define fseek(f,o,w) \
     (GL_LINK_WARNING ("fseek cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use fseeko function for handling of large files"), \
      fseek (f, o, w))
# endif
#endif

#if 1
# if 0
#  define ftello rpl_ftello
extern off_t ftello (FILE *fp);
#  define ftell(fp) ftello (fp)
# endif
#elif defined GNULIB_POSIXCHECK
# undef ftello
# define ftello(f) \
   (GL_LINK_WARNING ("ftello is unportable - " \
                     "use gnulib module ftello for portability"), \
    ftello (f))
#endif

#if 0 && 0
extern long rpl_ftell (FILE *fp);
# undef ftell
# if GNULIB_POSIXCHECK
#  define ftell(f) \
     (GL_LINK_WARNING ("ftell cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use ftello function for handling of large files"), \
      rpl_ftell (f))
# else
#  define ftell rpl_ftell
# endif
#elif defined GNULIB_POSIXCHECK
# ifndef ftell
#  define ftell(f) \
     (GL_LINK_WARNING ("ftell cannot handle files larger than 4 GB " \
                       "on 32-bit platforms - " \
                       "use ftello function for handling of large files"), \
      ftell (f))
# endif
#endif

#if 1
# if 0
#  define fflush rpl_fflush
  /* Flush all pending data on STREAM according to POSIX rules.  Both
     output and seekable input streams are supported.
     Note! LOSS OF DATA can occur if fflush is applied on an input stream
     that is _not_seekable_ or on an update stream that is _not_seekable_
     and in which the most recent operation was input.  Seekability can
     be tested with lseek(fileno(fp),0,SEEK_CUR).  */
  extern int fflush (FILE *gl_stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef fflush
# define fflush(f) \
   (GL_LINK_WARNING ("fflush is not always POSIX compliant - " \
                     "use gnulib module fflush for portable " \
                     "POSIX compliance"), \
    fflush (f))
#endif

#if 1
# if !0
/* Read input, up to (and including) the next occurrence of DELIMITER, from
   STREAM, store it in *LINEPTR (and NUL-terminate it).
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getdelim (char **lineptr, size_t *linesize, int delimiter,
			 FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef getdelim
# define getdelim(l, s, d, f)					    \
  (GL_LINK_WARNING ("getdelim is unportable - "			    \
		    "use gnulib module getdelim for portability"),  \
   getdelim (l, s, d, f))
#endif

#if 1
# if 0
#  undef getline
#  define getline rpl_getline
# endif
# if !0 || 0
/* Read a line, up to (and including) the next newline, from STREAM, store it
   in *LINEPTR (and NUL-terminate it).
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getline (char **lineptr, size_t *linesize, FILE *stream);
# endif
#elif defined GNULIB_POSIXCHECK
# undef getline
# define getline(l, s, f)						\
  (GL_LINK_WARNING ("getline is unportable - "				\
		    "use gnulib module getline for portability"),	\
   getline (l, s, f))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _GL_STDIO_H */
#endif /* _GL_STDIO_H */
#endif
