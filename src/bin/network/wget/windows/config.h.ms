/* Configuration header file.
   Copyright (C) 1995, 1996, 1997, 1998, 2004
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   In addition, as a special exception, the Free Software Foundation
   gives permission to link the code of its release of Wget with the
   OpenSSL project's "OpenSSL" library (or with modified versions of it
   that use the same license as the "OpenSSL" library), and distribute
   the linked executables.  You must obey the GNU General Public License
   in all respects for all of the code used other than "OpenSSL".  If you
   modify this file, you may extend this exception to your version of the
   file, but you are not obligated to do so.  If you do not wish to do
   so, delete this exception statement from your version.  */


#ifndef CONFIG_H
#define CONFIG_H

/* Define if you have the <alloca.h> header file.  */
#undef HAVE_ALLOCA_H

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE int

/* Define if your architecture is big endian (with the most
   significant byte first).  */
#undef WORDS_BIGENDIAN

/* Define if you want the FTP support for Opie compiled in.  */
#define ENABLE_OPIE 1

/* Define if you want the HTTP Digest Authorization compiled in.  */
#define ENABLE_DIGEST 1

/* Define if you want the NTLM authorization support compiled in. */
#ifdef HAVE_SSL
# define ENABLE_NTLM
#endif

/* Define if you want the debug output support compiled in.  */
#define ENABLE_DEBUG 1

/* Define if you have sys/time.h header. */
#undef HAVE_SYS_TIME_H

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#undef TIME_WITH_SYS_TIME

/* Define if you have struct utimbuf.  */
#define HAVE_STRUCT_UTIMBUF 1

/* Define if you have the gethostbyname function.  */
/* #undef HAVE_GETHOSTBYNAME */

/* Define if you have the uname function.  */
#undef HAVE_UNAME

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the gettimeofday function.  */
#undef HAVE_GETTIMEOFDAY

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the sys/utsname.h header. */
#undef HAVE_SYS_UTSNAME_H

/* Define if you have the strerror function. */
#define HAVE_STRERROR 1

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define if you have the strstr function. */
#define HAVE_STRSTR 1

/* Define if you have the strcasecmp function. */
#define HAVE_STRCASECMP 1

/* Define if you have the strncasecmp function. */
#define HAVE_STRNCASECMP 1

/* Define if you have the strptime function. */
#undef HAVE_STRPTIME

/* Define if you have the mktime function. */
#define HAVE_MKTIME 1

/* Define if you have the symlink function. */
#undef HAVE_SYMLINK

/* Define if you have the signal function. */
#undef HAVE_SIGNAL

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Use ANSI-style stdargs regardless of whether the compiler bothers
   to define __STDC__.  (Many don't when extensions are enabled.)
   This define used to be in mswindows.h, but wheren't making any use there */
#define WGET_USE_STDARG

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <unistd.h> header file.  */
#if !defined(_MSC_VER)
#undef HAVE_UNISTD_H
#endif

/* None except Digital Mars have sleep/usleep functions */
#if defined(__DMC__)
#define HAVE_USLEEP 1
#define HAVE_SLEEP 1
#endif

/* Define if you have the <utime.h> header file.  */
#undef HAVE_UTIME_H

/* Define if you have the <sys/utime.h> header file.  */
#define HAVE_SYS_UTIME_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the <pwd.h> header file.  */
#undef HAVE_PWD_H

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H 1

/* Define to be the name of the operating system.  */
#define OS_TYPE "Windows"

#define CTRLBREAK_BACKGND 1

/* Define if you wish to compile with socks support.  */
/* #undef HAVE_SOCKS */

/* Define to 1 if ANSI function prototypes are usable.  */
#define PROTOTYPES 1

/* Define if we're compiling in support for MD5.  */
#define HAVE_MD5 1

/* Define if we're using Solaris libmd5.  */
#undef HAVE_SOLARIS_MD5

/* Define if we're using builtin (GNU) md5.c.  */
#define HAVE_BUILTIN_MD5 1

/* Work around VC 6 issues */
#define inline __inline
#define ftruncate chsize

/* Define if you have the isatty function.  */
#define HAVE_ISATTY 1

/* Define if you have the memmove function */
#define HAVE_MEMMOVE 1

/* Define to the length of short. */
#define SIZEOF_SHORT 2

/* Define to the length of int. */
#define SIZEOF_INT 4

/* Define to the length of long. */
#define SIZEOF_LONG 4

/* Defined to int or size_t on systems without socklen_t.  */
#define socklen_t int

/* Define if you have uint32_t.  */
#undef HAVE_UINT32_T

#endif /* CONFIG_H */
