/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A more-standard <time.h>.

   Copyright (C) 2007-2009 Free Software Foundation, Inc.

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

/* Don't get in the way of glibc when it includes time.h merely to
   declare a few standard symbols, rather than to declare all the
   symbols.  Also, Solaris 8 <time.h> eventually includes itself
   recursively; if that is happening, just include the system <time.h>
   without adding our own declarations.  */
#if (defined __need_time_t || defined __need_clock_t \
     || defined __need_timespec \
     || defined _GL_TIME_H)

# @INCLUDE_NEXT@ @NEXT_TIME_H@

#else

# define _GL_TIME_H

# @INCLUDE_NEXT@ @NEXT_TIME_H@

/* NetBSD 5.0 mis-defines NULL.  */
#include <stddef.h>

# ifdef __cplusplus
extern "C" {
# endif

/* Some systems don't define struct timespec (e.g., AIX 4.1, Ultrix 4.3).
   Or they define it with the wrong member names or define it in <sys/time.h>
   (e.g., FreeBSD circa 1997).  */
# if ! @TIME_H_DEFINES_STRUCT_TIMESPEC@
#  if @SYS_TIME_H_DEFINES_STRUCT_TIMESPEC@
#   include <sys/time.h>
#  else
#   undef timespec
#   define timespec rpl_timespec
struct timespec
{
  time_t tv_sec;
  long int tv_nsec;
};
#  endif
# endif

/* Sleep for at least RQTP seconds unless interrupted,  If interrupted,
   return -1 and store the remaining time into RMTP.  See
   <http://www.opengroup.org/susv3xsh/nanosleep.html>.  */
# if @REPLACE_NANOSLEEP@
#  define nanosleep rpl_nanosleep
int nanosleep (struct timespec const *__rqtp, struct timespec *__rmtp);
# endif

/* Return the 'time_t' representation of TP and normalize TP.  */
# if @REPLACE_MKTIME@
#  define mktime rpl_mktime
extern time_t mktime (struct tm *__tp);
# endif

/* Convert TIMER to RESULT, assuming local time and UTC respectively.  See
   <http://www.opengroup.org/susv3xsh/localtime_r.html> and
   <http://www.opengroup.org/susv3xsh/gmtime_r.html>.  */
# if @REPLACE_LOCALTIME_R@
#  undef localtime_r
#  define localtime_r rpl_localtime_r
#  undef gmtime_r
#  define gmtime_r rpl_gmtime_r
struct tm *localtime_r (time_t const *restrict __timer,
			struct tm *restrict __result);
struct tm *gmtime_r (time_t const *restrict __timer,
		     struct tm *restrict __result);
# endif

/* Parse BUF as a time stamp, assuming FORMAT specifies its layout, and store
   the resulting broken-down time into TM.  See
   <http://www.opengroup.org/susv3xsh/strptime.html>.  */
# if @REPLACE_STRPTIME@
#  undef strptime
#  define strptime rpl_strptime
char *strptime (char const *restrict __buf, char const *restrict __format,
		struct tm *restrict __tm);
# endif

/* Convert TM to a time_t value, assuming UTC.  */
# if @REPLACE_TIMEGM@
#  undef timegm
#  define timegm rpl_timegm
time_t timegm (struct tm *__tm);
# endif

/* Encourage applications to avoid unsafe functions that can overrun
   buffers when given outlandish struct tm values.  Portable
   applications should use strftime (or even sprintf) instead.  */
# if GNULIB_PORTCHECK
#  undef asctime
#  define asctime eschew_asctime
#  undef asctime_r
#  define asctime_r eschew_asctime_r
#  undef ctime
#  define ctime eschew_ctime
#  undef ctime_r
#  define ctime_r eschew_ctime_r
# endif

# ifdef __cplusplus
}
# endif

#endif
