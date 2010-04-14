/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A GNU-like <stdio.h>.

   Copyright (C) 2004, 2007-2010 Free Software Foundation, Inc.

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

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if defined __need_FILE || defined __need___FILE
/* Special invocation convention inside glibc header files.  */

#@INCLUDE_NEXT@ @NEXT_STDIO_H@

#else
/* Normal invocation convention.  */

#ifndef _GL_STDIO_H

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_STDIO_H@

#ifndef _GL_STDIO_H
#define _GL_STDIO_H

/* Get va_list.  Needed on many systems, including glibc 2.8.  */
#include <stdarg.h>

#include <stddef.h>

/* Get off_t and ssize_t.  Needed on many systems, including glibc 2.8.  */
#include <sys/types.h>

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
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

/* The definition of _GL_ARG_NONNULL is copied here.  */

/* The definition of _GL_WARN_ON_USE is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif

#if @GNULIB_DPRINTF@
# if @REPLACE_DPRINTF@
#  define dprintf rpl_dprintf
# endif
# if @REPLACE_DPRINTF@ || !@HAVE_DPRINTF@
extern int dprintf (int fd, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3))) _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef dprintf
# define dprintf \
    (GL_LINK_WARNING ("dprintf is unportable - " \
                      "use gnulib module dprintf for portability"), \
     dprintf)
#endif

#if @GNULIB_FCLOSE@
# if @REPLACE_FCLOSE@
#  define fclose rpl_fclose
  /* Close STREAM and its underlying file descriptor.  */
extern int fclose (FILE *stream) _GL_ARG_NONNULL ((1));
# endif
#elif defined GNULIB_POSIXCHECK
# undef fclose
# define fclose(f) \
   (GL_LINK_WARNING ("fclose is not always POSIX compliant - " \
                     "use gnulib module fclose for portable " \
                     "POSIX compliance"), \
    fclose (f))
#endif

#if @GNULIB_FFLUSH@
# if @REPLACE_FFLUSH@
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

/* It is very rare that the developer ever has full control of stdin,
   so any use of gets warrants an unconditional warning.  Assume it is
   always declared, since it is required by C89.  */
#undef gets
_GL_WARN_ON_USE (gets, "gets is a security hole - use fgets instead");

#if @GNULIB_FOPEN@
# if @REPLACE_FOPEN@
#  undef fopen
#  define fopen rpl_fopen
extern FILE * fopen (const char *filename, const char *mode)
     _GL_ARG_NONNULL ((1, 2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef fopen
# define fopen(f,m) \
   (GL_LINK_WARNING ("fopen on Win32 platforms is not POSIX compatible - " \
                     "use gnulib module fopen for portability"), \
    fopen (f, m))
#endif

#if @GNULIB_FPRINTF_POSIX@
# if @REPLACE_FPRINTF@
#  define fprintf rpl_fprintf
extern int fprintf (FILE *fp, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)))
       _GL_ARG_NONNULL ((1, 2));
# endif
#elif @GNULIB_FPRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# define fprintf rpl_fprintf
extern int fprintf (FILE *fp, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)))
       _GL_ARG_NONNULL ((1, 2));
#elif defined GNULIB_POSIXCHECK
# undef fprintf
# define fprintf \
    (GL_LINK_WARNING ("fprintf is not always POSIX compliant - " \
                      "use gnulib module fprintf-posix for portable " \
                      "POSIX compliance"), \
     fprintf)
#endif

#if @GNULIB_FPURGE@
# if @REPLACE_FPURGE@
#  define fpurge rpl_fpurge
# endif
# if @REPLACE_FPURGE@ || !@HAVE_DECL_FPURGE@
  /* Discard all pending buffered I/O data on STREAM.
     STREAM must not be wide-character oriented.
     When discarding pending output, the file position is set back to where it
     was before the write calls.  When discarding pending input, the file
     position is advanced to match the end of the previously read input.
     Return 0 if successful.  Upon error, return -1 and set errno.  */
  extern int fpurge (FILE *gl_stream) _GL_ARG_NONNULL ((1));
# endif
#elif defined GNULIB_POSIXCHECK
# undef fpurge
# define fpurge(f) \
   (GL_LINK_WARNING ("fpurge is not always present - " \
                     "use gnulib module fpurge for portability"), \
    fpurge (f))
#endif

#if @GNULIB_FPUTC@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef fputc
# define fputc rpl_fputc
extern int fputc (int c, FILE *stream) _GL_ARG_NONNULL ((2));
#endif

#if @GNULIB_FPUTS@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef fputs
# define fputs rpl_fputs
extern int fputs (const char *string, FILE *stream) _GL_ARG_NONNULL ((1, 2));
#endif

#if @GNULIB_FREOPEN@
# if @REPLACE_FREOPEN@
#  undef freopen
#  define freopen rpl_freopen
extern FILE * freopen (const char *filename, const char *mode, FILE *stream)
     _GL_ARG_NONNULL ((2, 3));
# endif
#elif defined GNULIB_POSIXCHECK
# undef freopen
# define freopen(f,m,s) \
   (GL_LINK_WARNING ("freopen on Win32 platforms is not POSIX compatible - " \
                     "use gnulib module freopen for portability"), \
    freopen (f, m, s))
#endif

/* Set up the following warnings, based on which modules are in use.
   GNU Coding Standards discourage the use of fseek, since it imposes
   an arbitrary limitation on some 32-bit hosts.  Remember that the
   fseek module depends on the fseeko module, so we only have three
   cases to consider:

   1. The developer is not using either module.  Issue a warning under
   GNULIB_POSIXCHECK for both functions, to remind them that both
   functions have bugs on some systems.  _GL_NO_LARGE_FILES has no
   impact on this warning.

   2. The developer is using both modules.  They may be unaware of the
   arbitrary limitations of fseek, so issue a warning under
   GNULIB_POSIXCHECK.  On the other hand, they may be using both
   modules intentionally, so the developer can define
   _GL_NO_LARGE_FILES in the compilation units where the use of fseek
   is safe, to silence the warning.

   3. The developer is using the fseeko module, but not fseek.  Gnulib
   guarantees that fseek will still work around platform bugs in that
   case, but we presume that the developer is aware of the pitfalls of
   fseek and was trying to avoid it, so issue a warning even when
   GNULIB_POSIXCHECK is undefined.  Again, _GL_NO_LARGE_FILES can be
   defined to silence the warning in particular compilation units.

   Most gnulib clients that perform stream operations should fall into
   category three.  */

#if @GNULIB_FSEEK@
# if defined GNULIB_POSIXCHECK && !defined _GL_NO_LARGE_FILES
#  define _GL_FSEEK_WARN /* Category 2, above.  */
#  undef fseek
# endif
# if @REPLACE_FSEEK@
#  undef fseek
#  define fseek rpl_fseek
extern int fseek (FILE *fp, long offset, int whence) _GL_ARG_NONNULL ((1));
# endif
#endif

#if @GNULIB_FSEEKO@
# if !@GNULIB_FSEEK@ && !defined _GL_NO_LARGE_FILES
#  define _GL_FSEEK_WARN /* Category 3, above.  */
#  undef fseek
# endif
# if @REPLACE_FSEEKO@
/* Provide fseek, fseeko functions that are aware of a preceding
   fflush(), and which detect pipes.  */
#  undef fseeko
#  define fseeko rpl_fseeko
extern int fseeko (FILE *fp, off_t offset, int whence) _GL_ARG_NONNULL ((1));
#  if !@GNULIB_FSEEK@
#   undef fseek
#   define fseek rpl_fseek
static inline int _GL_ARG_NONNULL ((1))
rpl_fseek (FILE *fp, long offset, int whence)
{
  return fseeko (fp, offset, whence);
}
#  endif
# endif
#elif defined GNULIB_POSIXCHECK
# define _GL_FSEEK_WARN /* Category 1, above.  */
# undef fseek
# undef fseeko
# if HAVE_RAW_DECL_FSEEKO
_GL_WARN_ON_USE (fseeko, "fseeko is unportable - "
                 "use gnulib module fseeko for portability");
# endif
#endif

#ifdef _GL_FSEEK_WARN
# undef _GL_FSEEK_WARN
/* Here, either fseek is undefined (but C89 guarantees that it is
   declared), or it is defined as rpl_fseek (declared above).  */
_GL_WARN_ON_USE (fseek, "fseek cannot handle files larger than 4 GB "
                 "on 32-bit platforms - "
                 "use fseeko function for handling of large files");
#endif

/* See the comments on fseek/fseeko.  */

#if @GNULIB_FTELL@
# if defined GNULIB_POSIXCHECK && !defined _GL_NO_LARGE_FILES
#  define _GL_FTELL_WARN /* Category 2, above.  */
#  undef ftell
# endif
# if && @REPLACE_FTELL@
#  undef ftell
#  define ftell rpl_ftell
extern long ftell (FILE *fp) _GL_ARG_NONNULL ((1));
# endif
#endif

#if @GNULIB_FTELLO@
# if !@GNULIB_FTELL@ && !defined _GL_NO_LARGE_FILES
#  define _GL_FTELL_WARN /* Category 3, above.  */
#  undef ftell
# endif
# if @REPLACE_FTELLO@
#  undef ftello
#  define ftello rpl_ftello
extern off_t ftello (FILE *fp) _GL_ARG_NONNULL ((1));
#  if !@GNULIB_FTELL@
#   undef ftell
#   define ftell rpl_ftell
static inline long _GL_ARG_NONNULL ((1))
rpl_ftell (FILE *f)
{
  return ftello (f);
}
#  endif
# endif
#elif defined GNULIB_POSIXCHECK
# define _GL_FTELL_WARN /* Category 1, above.  */
# undef ftell
# undef ftello
# if HAVE_RAW_DECL_FTELLO
_GL_WARN_ON_USE (ftello, "ftello is unportable - "
                 "use gnulib module ftello for portability");
# endif
#endif

#ifdef _GL_FTELL_WARN
# undef _GL_FTELL_WARN
/* Here, either ftell is undefined (but C89 guarantees that it is
   declared), or it is defined as rpl_ftell (declared above).  */
_GL_WARN_ON_USE (ftell, "ftell cannot handle files larger than 4 GB "
                 "on 32-bit platforms - "
                 "use ftello function for handling of large files");
#endif

#if @GNULIB_FWRITE@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef fwrite
# define fwrite rpl_fwrite
extern size_t fwrite (const void *ptr, size_t s, size_t n, FILE *stream)
     _GL_ARG_NONNULL ((1, 4));
#endif

#if @GNULIB_GETDELIM@
# if !@HAVE_DECL_GETDELIM@
/* Read input, up to (and including) the next occurrence of DELIMITER, from
   STREAM, store it in *LINEPTR (and NUL-terminate it).
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getdelim (char **lineptr, size_t *linesize, int delimiter,
                         FILE *stream)
     _GL_ARG_NONNULL ((1, 2, 4));
# endif
#elif defined GNULIB_POSIXCHECK
# undef getdelim
# define getdelim(l, s, d, f)                                       \
  (GL_LINK_WARNING ("getdelim is unportable - "                     \
                    "use gnulib module getdelim for portability"),  \
   getdelim (l, s, d, f))
#endif

#if @GNULIB_GETLINE@
# if @REPLACE_GETLINE@
#  undef getline
#  define getline rpl_getline
# endif
# if !@HAVE_DECL_GETLINE@ || @REPLACE_GETLINE@
/* Read a line, up to (and including) the next newline, from STREAM, store it
   in *LINEPTR (and NUL-terminate it).
   *LINEPTR is a pointer returned from malloc (or NULL), pointing to *LINESIZE
   bytes of space.  It is realloc'd as necessary.
   Return the number of bytes read and stored at *LINEPTR (not including the
   NUL terminator), or -1 on error or EOF.  */
extern ssize_t getline (char **lineptr, size_t *linesize, FILE *stream)
     _GL_ARG_NONNULL ((1, 2, 3));
# endif
#elif defined GNULIB_POSIXCHECK
# undef getline
# define getline(l, s, f)                                               \
  (GL_LINK_WARNING ("getline is unportable - "                          \
                    "use gnulib module getline for portability"),       \
   getline (l, s, f))
#endif

#if @GNULIB_OBSTACK_PRINTF@
# if @REPLACE_OBSTACK_PRINTF@
#  define obstack_printf rpl_osbtack_printf
#  define obstack_vprintf rpl_obstack_vprintf
# endif
# if @REPLACE_OBSTACK_PRINTF@ || !@HAVE_DECL_OBSTACK_PRINTF@
  struct obstack;
  /* Grow an obstack with formatted output.  Return the number of
     bytes added to OBS.  No trailing nul byte is added, and the
     object should be closed with obstack_finish before use.  Upon
     memory allocation error, call obstack_alloc_failed_handler.  Upon
     other error, return -1.  */
  extern int obstack_printf (struct obstack *obs, const char *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3))) _GL_ARG_NONNULL ((1, 2));
  extern int obstack_vprintf (struct obstack *obs, const char *format,
                              va_list args)
    __attribute__ ((__format__ (__printf__, 2, 0))) _GL_ARG_NONNULL ((1, 2));
# endif
#endif

#if @GNULIB_PERROR@
# if @REPLACE_PERROR@
#  define perror rpl_perror
/* Print a message to standard error, describing the value of ERRNO,
   (if STRING is not NULL and not empty) prefixed with STRING and ": ",
   and terminated with a newline.  */
extern void perror (const char *string);
# endif
#elif defined GNULIB_POSIXCHECK
# undef perror
# define perror(s) \
    (GL_LINK_WARNING ("perror is not always POSIX compliant - " \
                      "use gnulib module perror for portability"), \
     perror (s))
#endif

#if @GNULIB_POPEN@
# if @REPLACE_POPEN@
#  undef popen
#  define popen rpl_popen
extern FILE *popen (const char *cmd, const char *mode)
     _GL_ARG_NONNULL ((1, 2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef popen
# define popen(c,m) \
   (GL_LINK_WARNING ("popen is buggy on some platforms - " \
                     "use gnulib module popen or pipe for more portability"), \
    popen (c, m))
#endif

#if @GNULIB_PRINTF_POSIX@
# if @REPLACE_PRINTF@
/* Don't break __attribute__((format(printf,M,N))).  */
#  define printf __printf__
extern int printf (const char *format, ...)
       __attribute__ ((__format__ (__printf__, 1, 2))) _GL_ARG_NONNULL ((1));
# endif
#elif @GNULIB_PRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
/* Don't break __attribute__((format(printf,M,N))).  */
# define printf __printf__
extern int printf (const char *format, ...)
       __attribute__ ((__format__ (__printf__, 1, 2))) _GL_ARG_NONNULL ((1));
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

#if @GNULIB_PUTC@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef putc
# define putc rpl_fputc
extern int putc (int c, FILE *stream) _GL_ARG_NONNULL ((2));
#endif

#if @GNULIB_PUTCHAR@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef putchar
# define putchar rpl_putchar
extern int putchar (int c);
#endif

#if @GNULIB_PUTS@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# undef puts
# define puts rpl_puts
extern int puts (const char *string) _GL_ARG_NONNULL ((1));
#endif

#if @GNULIB_REMOVE@
# if @REPLACE_REMOVE@
#  undef remove
#  define remove rpl_remove
extern int remove (const char *name) _GL_ARG_NONNULL ((1));
# endif
#elif defined GNULIB_POSIXCHECK
# undef remove
# define remove(n)                                         \
   (GL_LINK_WARNING ("remove cannot handle directories on some platforms - " \
                     "use gnulib module remove for more portability"), \
    remove (n))
#endif

#if @GNULIB_RENAME@
# if @REPLACE_RENAME@
#  undef rename
#  define rename rpl_rename
extern int rename (const char *old_filename, const char *new_filename)
     _GL_ARG_NONNULL ((1, 2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef rename
# define rename(o,n)                                       \
   (GL_LINK_WARNING ("rename is buggy on some platforms - " \
                     "use gnulib module rename for more portability"), \
    rename (o, n))
#endif

#if @GNULIB_RENAMEAT@
# if @REPLACE_RENAMEAT@
#  undef renameat
#  define renameat rpl_renameat
# endif
# if !@HAVE_RENAMEAT@ || @REPLACE_RENAMEAT@
extern int renameat (int fd1, char const *file1, int fd2, char const *file2)
     _GL_ARG_NONNULL ((2, 4));
# endif
#elif defined GNULIB_POSIXCHECK
# undef renameat
# define renameat(d1,f1,d2,f2)             \
    (GL_LINK_WARNING ("renameat is not portable - " \
                      "use gnulib module renameat for portability"), \
     renameat (d1, f1, d2, f2))
#endif

#if @GNULIB_SNPRINTF@
# if @REPLACE_SNPRINTF@
#  define snprintf rpl_snprintf
# endif
# if @REPLACE_SNPRINTF@ || !@HAVE_DECL_SNPRINTF@
extern int snprintf (char *str, size_t size, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 3, 4)))
       _GL_ARG_NONNULL ((3));
# endif
#elif defined GNULIB_POSIXCHECK
# undef snprintf
# define snprintf \
    (GL_LINK_WARNING ("snprintf is unportable - " \
                      "use gnulib module snprintf for portability"), \
     snprintf)
#endif

/* Some people would argue that sprintf should be handled like gets
   (for example, OpenBSD issues a link warning for both functions),
   since both can cause security holes due to buffer overruns.
   However, we believe that sprintf can be used safely, and is more
   efficient than snprintf in those safe cases; and as proof of our
   belief, we use sprintf in several gnulib modules.  So this header
   intentionally avoids adding a warning to sprintf except when
   GNULIB_POSIXCHECK is defined.  */

#if @GNULIB_SPRINTF_POSIX@
# if @REPLACE_SPRINTF@
#  define sprintf rpl_sprintf
extern int sprintf (char *str, const char *format, ...)
       __attribute__ ((__format__ (__printf__, 2, 3)))
       _GL_ARG_NONNULL ((1, 2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef sprintf
# define sprintf \
    (GL_LINK_WARNING ("sprintf is not always POSIX compliant - " \
                      "use gnulib module sprintf-posix for portable " \
                      "POSIX compliance"), \
     sprintf)
#endif

#if @GNULIB_VASPRINTF@
# if @REPLACE_VASPRINTF@
#  define asprintf rpl_asprintf
#  define vasprintf rpl_vasprintf
# endif
# if @REPLACE_VASPRINTF@ || !@HAVE_VASPRINTF@
  /* Write formatted output to a string dynamically allocated with malloc().
     If the memory allocation succeeds, store the address of the string in
     *RESULT and return the number of resulting bytes, excluding the trailing
     NUL.  Upon memory allocation error, or some other error, return -1.  */
  extern int asprintf (char **result, const char *format, ...)
    __attribute__ ((__format__ (__printf__, 2, 3))) _GL_ARG_NONNULL ((1, 2));
  extern int vasprintf (char **result, const char *format, va_list args)
    __attribute__ ((__format__ (__printf__, 2, 0))) _GL_ARG_NONNULL ((1, 2));
# endif
#endif

#if @GNULIB_VDPRINTF@
# if @REPLACE_VDPRINTF@
#  define vdprintf rpl_vdprintf
# endif
# if @REPLACE_VDPRINTF@ || !@HAVE_VDPRINTF@
extern int vdprintf (int fd, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0))) _GL_ARG_NONNULL ((2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vdprintf
# define vdprintf(d,f,a) \
    (GL_LINK_WARNING ("vdprintf is unportable - " \
                      "use gnulib module vdprintf for portability"), \
     vdprintf (d, f, a))
#endif

#if @GNULIB_VFPRINTF_POSIX@
# if @REPLACE_VFPRINTF@
#  define vfprintf rpl_vfprintf
extern int vfprintf (FILE *fp, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)))
       _GL_ARG_NONNULL ((1, 2));
# endif
#elif @GNULIB_VFPRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# define vfprintf rpl_vfprintf
extern int vfprintf (FILE *fp, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)))
       _GL_ARG_NONNULL ((1, 2));
#elif defined GNULIB_POSIXCHECK
# undef vfprintf
# define vfprintf(s,f,a) \
    (GL_LINK_WARNING ("vfprintf is not always POSIX compliant - " \
                      "use gnulib module vfprintf-posix for portable " \
                      "POSIX compliance"), \
     vfprintf (s, f, a))
#endif

#if @GNULIB_VPRINTF_POSIX@
# if @REPLACE_VPRINTF@
#  define vprintf rpl_vprintf
extern int vprintf (const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 1, 0))) _GL_ARG_NONNULL ((1));
# endif
#elif @GNULIB_VPRINTF@ && @REPLACE_STDIO_WRITE_FUNCS@ && @GNULIB_STDIO_H_SIGPIPE@
# define vprintf rpl_vprintf
extern int vprintf (const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 1, 0))) _GL_ARG_NONNULL ((1));
#elif defined GNULIB_POSIXCHECK
# undef vprintf
# define vprintf(f,a) \
    (GL_LINK_WARNING ("vprintf is not always POSIX compliant - " \
                      "use gnulib module vprintf-posix for portable " \
                      "POSIX compliance"), \
     vprintf (f, a))
#endif

#if @GNULIB_VSNPRINTF@
# if @REPLACE_VSNPRINTF@
#  define vsnprintf rpl_vsnprintf
# endif
# if @REPLACE_VSNPRINTF@ || !@HAVE_DECL_VSNPRINTF@
extern int vsnprintf (char *str, size_t size, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 3, 0)))
       _GL_ARG_NONNULL ((3));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vsnprintf
# define vsnprintf(b,s,f,a) \
    (GL_LINK_WARNING ("vsnprintf is unportable - " \
                      "use gnulib module vsnprintf for portability"), \
     vsnprintf (b, s, f, a))
#endif

#if @GNULIB_VSPRINTF_POSIX@
# if @REPLACE_VSPRINTF@
#  define vsprintf rpl_vsprintf
extern int vsprintf (char *str, const char *format, va_list args)
       __attribute__ ((__format__ (__printf__, 2, 0)))
       _GL_ARG_NONNULL ((1, 2));
# endif
#elif defined GNULIB_POSIXCHECK
# undef vsprintf
# define vsprintf(b,f,a) \
    (GL_LINK_WARNING ("vsprintf is not always POSIX compliant - " \
                      "use gnulib module vsprintf-posix for portable " \
                      "POSIX compliance"), \
     vsprintf (b, f, a))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _GL_STDIO_H */
#endif /* _GL_STDIO_H */
#endif
