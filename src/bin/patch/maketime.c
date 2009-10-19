/* Convert struct partime into time_t.  */

/* Copyright 1992, 1993, 1994, 1995, 1997 Paul Eggert
   Distributed under license by the Free Software Foundation, Inc.

   This file is part of RCS.

   RCS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   RCS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with RCS; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Report problems and direct all questions to:

	rcs-bugs@cs.purdue.edu

 */

/* For maximum portability, use only localtime and gmtime.
   Make no assumptions about the time_t epoch or the range of time_t values.
   Avoid mktime because it's not universal and because there's no easy,
   portable way for mktime to yield the inverse of gmtime.  */

#if has_conf_h
# include <conf.h>
#else
# if HAVE_CONFIG_H
#  include <config.h>
# else
#  ifndef __STDC__
#   define const
#  endif
# endif
  /* MIPS RISCOS4.52 defines time_t in <sys/types.h> not <time.h>.  */
# include <sys/types.h>
# if HAVE_LIMITS_H
#  include <limits.h>
# endif
# ifndef LONG_MIN
# define LONG_MIN (-1-2147483647L)
# endif
# if STDC_HEADERS
#  include <stdlib.h>
# endif
# include <time.h>
# ifdef __STDC__
#  define P(x) x
# else
#  define P(x) ()
# endif
#endif

#include <partime.h>
#include <maketime.h>

char const maket_id[] =
  "$Id: maketime.c 8008 2004-06-16 21:22:10Z korli $";

static int isleap P ((int));
static int month_days P ((struct tm const *));
static time_t maketime P ((struct partime const *, time_t));

/* Suppose A1 + B1 = SUM1, using 2's complement arithmetic ignoring overflow.
   Suppose A, B and SUM have the same respective signs as A1, B1, and SUM1.
   Then this yields nonzero if overflow occurred during the addition.
   Overflow occurs if A and B have the same sign, but A and SUM differ in sign.
   Use `^' to test whether signs differ, and `< 0' to isolate the sign.  */
#define overflow_sum_sign(a, b, sum) ((~((a) ^ (b)) & ((a) ^ (sum))) < 0)

/* Quotient and remainder when dividing A by B,
   truncating towards minus infinity, where B is positive.  */
#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define MOD(a, b) ((a) % (b) + (b) * ((a) % (b) < 0))

/* Number of days in 400 consecutive Gregorian years.  */
#define Y400_DAYS (365 * 400L + 100 - 4 + 1)

/* Number of years to add to tm_year to get Gregorian year.  */
#define TM_YEAR_ORIGIN 1900

static int
isleap (y)
     int y;
{
  return (y & 3) == 0 && (y % 100 != 0 || y % 400 == 0);
}

/* days in year before start of months 0-12 */
static int const month_yday[] =
{
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

/* Yield the number of days in TM's month.  */
static int
month_days (tm)
     struct tm const *tm;
{
  int m = tm->tm_mon;
  return (month_yday[m + 1] - month_yday[m]
	  + (m == 1 && isleap (tm->tm_year + TM_YEAR_ORIGIN)));
}

/* Convert UNIXTIME to struct tm form.
   Use gmtime if available and if !LOCALZONE, localtime otherwise.  */
struct tm *
time2tm (unixtime, localzone)
     time_t unixtime;
     int localzone;
{
  struct tm *tm;
#ifdef TZ_is_unset
  static char const *TZ;
  if (!TZ && !(TZ = getenv ("TZ")))
    TZ_is_unset ("The TZ environment variable is not set; please set it to your timezone");
#endif
  if (localzone || !(tm = gmtime (&unixtime)))
    tm = localtime (&unixtime);
  return tm;
}

/* Yield A - B, measured in seconds.  */
time_t
difftm (a, b)
     struct tm const *a;
     struct tm const *b;
{
  int ay = a->tm_year + (TM_YEAR_ORIGIN - 1);
  int by = b->tm_year + (TM_YEAR_ORIGIN - 1);
  int ac = DIV (ay, 100);
  int bc = DIV (by, 100);
  int difference_in_day_of_year = a->tm_yday - b->tm_yday;
  int intervening_leap_days = (((ay >> 2) - (by >> 2))
			       - (ac - bc)
			       + ((ac >> 2) - (bc >> 2)));
  time_t difference_in_years = ay - by;
  time_t difference_in_days
    = (difference_in_years * 365
       + (intervening_leap_days + difference_in_day_of_year));
  return (((((difference_in_days * 24
	      + (a->tm_hour - b->tm_hour))
	     * 60)
	    + (a->tm_min - b->tm_min))
	   * 60)
	  + (a->tm_sec - b->tm_sec));
}

/* Adjust time T by adding SECONDS.
   The absolute value of SECONDS cannot exceed 59 * INT_MAX,
   and also cannot exceed one month's worth of seconds;
   this is enough to handle any POSIX or real-life daylight-saving offset.
   Adjust only T's year, mon, mday, hour, min and sec members;
   plus adjust wday if it is defined.  */
void
adjzone (t, seconds)
     register struct tm *t;
     long seconds;
{
  int days = 0;

  /* This code can be off by a second if SECONDS is not a multiple of 60,
     if T is local time, and if a leap second happens during this minute.
     But this bug has never occurred, and most likely will not ever occur.
     Liberia, the last country for which SECONDS % 60 was nonzero,
     switched to UTC in May 1972; the first leap second was in June 1972.  */
  int leap_second = t->tm_sec == 60;
  long sec = seconds + (t->tm_sec - leap_second);
  if (sec < 0)
    {
      if ((t->tm_min -= (59 - sec) / 60) < 0
	  && (t->tm_hour -= (59 - t->tm_min) / 60) < 0)
	{
	  days = - ((23 - t->tm_hour) / 24);
	  if ((t->tm_mday += days) <= 0)
	    {
	      if (--t->tm_mon < 0)
		{
		  --t->tm_year;
		  t->tm_mon = 11;
		}
	      t->tm_mday += month_days (t);
	    }
	}
    }
  else
    {
      if (60 <= (t->tm_min += sec / 60)
	  && (24 <= (t->tm_hour += t->tm_min / 60)))
	{
	  days = t->tm_hour / 24;
	  if (month_days (t) < (t->tm_mday += days))
	    {
	      if (11 < ++t->tm_mon)
		{
		  ++t->tm_year;
		  t->tm_mon = 0;
		}
	      t->tm_mday = 1;
	    }
	}
    }
  if (TM_DEFINED (t->tm_wday))
    t->tm_wday = MOD (t->tm_wday + days, 7);
  t->tm_hour = MOD (t->tm_hour, 24);
  t->tm_min = MOD (t->tm_min, 60);
  t->tm_sec = (int) MOD (sec, 60) + leap_second;
}

/* Convert TM to time_t, using localtime if LOCALZONE and gmtime otherwise.
   Use only TM's year, mon, mday, hour, min, and sec members.
   Ignore TM's old tm_yday and tm_wday, but fill in their correct values.
   Yield -1 on failure (e.g. a member out of range).
   POSIX 1003.1 doesn't allow leap seconds, but some implementations
   have them anyway, so allow them if localtime/gmtime does.  */
time_t
tm2time (tm, localzone)
     struct tm *tm;
     int localzone;
{
  /* Cache the most recent t,tm pairs; 1 for gmtime, 1 for localtime.  */
  static time_t t_cache[2];
  static struct tm tm_cache[2];

  time_t d, gt;
  struct tm const *gtm;
  /* The maximum number of iterations should be enough to handle any
     combinations of leap seconds, time zone rule changes, and solar time.
     4 is probably enough; we use a bigger number just to be safe.  */
  int remaining_tries = 8;

  /* Avoid subscript errors.  */
  if (12 <= (unsigned) tm->tm_mon)
    return -1;

  tm->tm_yday = month_yday[tm->tm_mon] + tm->tm_mday
    - (tm->tm_mon < 2 || !isleap (tm->tm_year + TM_YEAR_ORIGIN));

  /* Make a first guess.  */
  gt = t_cache[localzone];
  gtm = gt ? &tm_cache[localzone] : time2tm (gt, localzone);

  /* Repeatedly use the error from the guess to improve the guess.  */
  while ((d = difftm (tm, gtm)) != 0)
    {
      if (--remaining_tries == 0)
	return -1;
      gt += d;
      gtm = time2tm (gt, localzone);
    }

  /* Check that the guess actually matches;
     overflow can cause difftm to yield 0 even on differing times,
     or tm may have members out of range (e.g. bad leap seconds).  */
#define TM_DIFFER(a,b) \
		( \
			((a)->tm_year ^ (b)->tm_year) | \
			((a)->tm_mon ^ (b)->tm_mon) | \
			((a)->tm_mday ^ (b)->tm_mday) | \
			((a)->tm_hour ^ (b)->tm_hour) | \
			((a)->tm_min ^ (b)->tm_min) | \
			((a)->tm_sec ^ (b)->tm_sec) \
		)
  if (TM_DIFFER (tm, gtm))
    {
      /* If gt is a leap second, try gt+1; if it is one greater than
         a leap second, try gt-1; otherwise, it doesn't matter.
         Leap seconds always fall at month end.  */
      int yd = tm->tm_year - gtm->tm_year;
      gt += yd + (yd ? 0 : tm->tm_mon - gtm->tm_mon);
      gtm = time2tm (gt, localzone);
      if (TM_DIFFER (tm, gtm))
	return -1;
    }
  t_cache[localzone] = gt;
  tm_cache[localzone] = *gtm;

  tm->tm_wday = gtm->tm_wday;
  return gt;
}

/* Check *PT and convert it to time_t.
   If it is incompletely specified, use DEFAULT_TIME to fill it out.
   Use localtime if PT->zone is the special value TM_LOCAL_ZONE.
   Yield -1 on failure.
   ISO 8601 day-of-year and week numbers are not yet supported.  */
static time_t
maketime (pt, default_time)
     struct partime const *pt;
     time_t default_time;
{
  int localzone, wday, year;
  struct tm tm;
  struct tm *tm0 = 0;
  time_t r;
  int use_ordinal_day;

  tm0 = 0;			/* Keep gcc -Wall happy.  */
  localzone = pt->zone == TM_LOCAL_ZONE;

  tm = pt->tm;
  year = tm.tm_year;
  wday = tm.tm_wday;
  use_ordinal_day = (!TM_DEFINED (tm.tm_mday)
		     && TM_DEFINED (wday) && TM_DEFINED (pt->wday_ordinal));

  if (use_ordinal_day || TM_DEFINED (pt->ymodulus) || !TM_DEFINED (year))
    {
      /* Get tm corresponding to default time.  */
      tm0 = time2tm (default_time, localzone);
      if (!localzone)
	adjzone (tm0, pt->zone);
    }

  if (use_ordinal_day)
    tm.tm_mday = (tm0->tm_mday
		  + ((wday - tm0->tm_wday + 7) % 7
		     + 7 * (pt->wday_ordinal - (pt->wday_ordinal != 0))));

  if (TM_DEFINED (pt->ymodulus))
    {
      /* Yield a year closest to the default that has the given modulus.  */
      int year0 = tm0->tm_year + TM_YEAR_ORIGIN;
      int y0 = MOD (year0, pt->ymodulus);
      int d = 2 * (year - y0);
      year += (((year0 - y0) / pt->ymodulus
	        + (pt->ymodulus < d ? -1 : d < -pt->ymodulus))
	       * pt->ymodulus);
    }
  else if (!TM_DEFINED (year))
    {
      /* Set default year, month, day from current time.  */
      year = tm0->tm_year + TM_YEAR_ORIGIN;
      if (!TM_DEFINED (tm.tm_mon))
	{
	  tm.tm_mon = tm0->tm_mon;
	  if (!TM_DEFINED (tm.tm_mday))
	    tm.tm_mday = tm0->tm_mday;
	}
    }

  /* Set remaining default fields to be their minimum values.  */
  if (!TM_DEFINED (tm.tm_mon))
    tm.tm_mon = 0;
  if (!TM_DEFINED (tm.tm_mday))
    tm.tm_mday = 1;
  if (!TM_DEFINED (tm.tm_hour))
    tm.tm_hour = 0;
  if (!TM_DEFINED (tm.tm_min))
    tm.tm_min = 0;
  if (!TM_DEFINED (tm.tm_sec))
    tm.tm_sec = 0;

  tm.tm_year = year - TM_YEAR_ORIGIN;
  if ((year < tm.tm_year) != (TM_YEAR_ORIGIN < 0))
    return -1;

  if (!localzone)
    {
      adjzone (&tm, -pt->zone);
      wday = tm.tm_wday;
    }

  /* Convert and fill in the rest of the tm.  */
  r = tm2time (&tm, localzone);
  if (r == -1)
    return r;

  /* Check weekday.  */
  if (TM_DEFINED (wday) && wday != tm.tm_wday)
    return -1;

  /* Add relative time, except for seconds.
     We handle seconds separately, at the end,
     so that leap seconds are handled properly.  */
  if (pt->tmr.tm_year | pt->tmr.tm_mon | pt->tmr.tm_mday
      | pt->tmr.tm_hour | pt->tmr.tm_min)
    {
      int years = tm.tm_year + pt->tmr.tm_year;
      int mons = tm.tm_mon + pt->tmr.tm_mon;
      int mdays = tm.tm_mday + pt->tmr.tm_mday;
      int hours = tm.tm_hour + pt->tmr.tm_hour;
      int mins = tm.tm_min + pt->tmr.tm_min;

      int carried_hours = DIV (mins, 60);
      int hours1 = hours + carried_hours;
      int carried_days = DIV (hours1, 24);
      int mdays1 = mdays + carried_days;

      int mon0 = MOD (mons, 12);
      int carried_years0 = DIV (mons, 12);
      int year0 = years + carried_years0;
      int yday0 = (month_yday[mon0]
		   - (mon0 < 2 || !isleap (year0 + TM_YEAR_ORIGIN)));

      int yday1 = yday0 + mdays1;
      int carried_years1 = DIV (yday1, Y400_DAYS) * 400;
      int year1 = year0 + carried_years1;
      int yday2 = MOD (yday1, Y400_DAYS);
      int leap;

      if (overflow_sum_sign (tm.tm_year, pt->tmr.tm_year, years)
	  | overflow_sum_sign (tm.tm_mon, pt->tmr.tm_mon, mons)
	  | overflow_sum_sign (tm.tm_mday, pt->tmr.tm_mday, mdays)
	  | overflow_sum_sign (tm.tm_hour, pt->tmr.tm_hour, hours)
	  | overflow_sum_sign (tm.tm_min, pt->tmr.tm_min, mins)
	  | overflow_sum_sign (hours, carried_hours, hours1)
	  | overflow_sum_sign (mdays, carried_days, mdays1)
	  | overflow_sum_sign (years, carried_years0, year0)
	  | overflow_sum_sign (yday0, mdays1, yday1)
	  | overflow_sum_sign (year0, carried_years1, year1))
	return -1;

      for (;;)
	{
	  int days_per_year = 365 + (leap = isleap (year1 + TM_YEAR_ORIGIN));
	  if (yday2 < days_per_year)
	    break;
	  yday2 -= days_per_year;
	  year1++;
	}

      tm.tm_year = year1;

      {
	int mon;
	for (mon = 11;
	     (tm.tm_mday = (yday2 - month_yday[mon] + (mon < 2 || !leap))) <= 0;
	     mon--)
	  continue;
	tm.tm_mon = mon;
      }

      tm.tm_hour = MOD (hours1, 24);
      tm.tm_min = MOD (mins, 60);

      r = tm2time (&tm, localzone);
      if (r == -1)
	return r;
    }

  /* Add the seconds' part of relative time.  */
  {
    time_t rs = r + pt->tmr.tm_sec;
    if ((pt->tmr.tm_sec < 0) != (rs < r))
      return -1;
    return rs;
  }
}

/* Parse a free-format date in *SOURCE, yielding a Unix format time.
   Update *SOURCE to point to the first character after the date.
   If *SOURCE is missing some information, take defaults from
   DEFAULT_TIME and DEFAULT_ZONE.  *SOURCE may even be the empty
   string or an immediately invalid string, in which case the default
   time and zone is used.
   Return (time_t) -1 if the time is invalid or cannot be represented.  */
time_t
str2time (source, default_time, default_zone)
     char const **source;
     time_t default_time;
     long default_zone;
{
  struct partime pt;

  *source = partime (*source, &pt);
  if (pt.zone == TM_UNDEFINED_ZONE)
    pt.zone = default_zone;
  return maketime (&pt, default_time);
}

#ifdef TEST
#include <stdio.h>
int
main (argc, argv)
     int argc;
     char **argv;
{
  time_t default_time = time ((time_t *) 0);
  long default_zone = argv[1] ? atol (argv[1]) : TM_LOCAL_ZONE;
  char buf[1000];
  while (fgets (buf, sizeof (buf), stdin))
    {
      char const *p = buf;
      time_t t = str2time (&p, default_time, default_zone);
      printf ("`%.*s' -> %s",
	      (int) (p - buf - (p[0] == '\0' && p[-1] == '\n')), buf,
	      asctime ((argv[1] ? gmtime : localtime) (&t)));
    }
  return 0;
}
#endif
