/* Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <byteswap.h>
#include <langinfo.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <sys/uio.h>

#include <assert.h>

#include "linereader.h"
#include "localedef.h"
#include "localeinfo.h"
#include "locfile.h"


/* Entry describing an entry of the era specification.  */
struct era_data
{
  int32_t direction;
  int32_t offset;
  int32_t start_date[3];
  int32_t stop_date[3];
  const char *name;
  const char *format;
  uint32_t *wname;
  uint32_t *wformat;
};


/* The real definition of the struct for the LC_TIME locale.  */
struct locale_time_t
{
  const char *abday[7];
  const uint32_t *wabday[7];
  int abday_defined;
  const char *day[7];
  const uint32_t *wday[7];
  int day_defined;
  const char *abmon[12];
  const uint32_t *wabmon[12];
  int abmon_defined;
  const char *mon[12];
  const uint32_t *wmon[12];
  int mon_defined;
  const char *am_pm[2];
  const uint32_t *wam_pm[2];
  int am_pm_defined;
  const char *d_t_fmt;
  const uint32_t *wd_t_fmt;
  const char *d_fmt;
  const uint32_t *wd_fmt;
  const char *t_fmt;
  const uint32_t *wt_fmt;
  const char *t_fmt_ampm;
  const uint32_t *wt_fmt_ampm;
  const char **era;
  const uint32_t **wera;
  uint32_t num_era;
  const char *era_year;
  const uint32_t *wera_year;
  const char *era_d_t_fmt;
  const uint32_t *wera_d_t_fmt;
  const char *era_t_fmt;
  const uint32_t *wera_t_fmt;
  const char *era_d_fmt;
  const uint32_t *wera_d_fmt;
  const char *alt_digits[100];
  const uint32_t *walt_digits[100];
  const char *date_fmt;
  const uint32_t *wdate_fmt;
  int alt_digits_defined;
  unsigned char week_ndays;
  uint32_t week_1stday;
  unsigned char week_1stweek;
  unsigned char first_weekday;
  unsigned char first_workday;
  unsigned char cal_direction;
  const char *timezone;
  const uint32_t *wtimezone;

  struct era_data *era_entries;
};


/* This constant is used to represent an empty wide character string.  */
static const uint32_t empty_wstr[1] = { 0 };


static void
time_startup (struct linereader *lr, struct localedef_t *locale,
	      int ignore_content)
{
  if (!ignore_content)
    locale->categories[LC_TIME].time =
      (struct locale_time_t *) xcalloc (1, sizeof (struct locale_time_t));

  if (lr != NULL)
    {
      lr->translate_strings = 1;
      lr->return_widestr = 1;
    }
}


void
time_finish (struct localedef_t *locale, struct charmap_t *charmap)
{
  struct locale_time_t *time = locale->categories[LC_TIME].time;
  int nothing = 0;

  /* Now resolve copying and also handle completely missing definitions.  */
  if (time == NULL)
    {
      /* First see whether we were supposed to copy.  If yes, find the
	 actual definition.  */
      if (locale->copy_name[LC_TIME] != NULL)
	{
	  /* Find the copying locale.  This has to happen transitively since
	     the locale we are copying from might also copying another one.  */
	  struct localedef_t *from = locale;

	  do
	    from = find_locale (LC_TIME, from->copy_name[LC_TIME],
				from->repertoire_name, charmap);
	  while (from->categories[LC_TIME].time == NULL
		 && from->copy_name[LC_TIME] != NULL);

	  time = locale->categories[LC_TIME].time
	    = from->categories[LC_TIME].time;
	}

      /* If there is still no definition issue an warning and create an
	 empty one.  */
      if (time == NULL)
	{
	  if (! be_quiet)
	    error (0, 0, _("No definition for %s category found"), "LC_TIME");
	  time_startup (NULL, locale, 0);
	  time = locale->categories[LC_TIME].time;
	  nothing = 1;
	}
    }

#define noparen(arg1, argn...) arg1, ##argn
#define TESTARR_ELEM(cat, val) \
  if (!time->cat##_defined)						      \
    {									      \
      const char *initval[] = { noparen val };				      \
      int i;								      \
									      \
      if (! be_quiet && ! nothing)					      \
	error (0, 0, _("%s: field `%s' not defined"), "LC_TIME", #cat);	      \
									      \
      for (i = 0; i < sizeof (initval) / sizeof (initval[0]); ++i)	      \
	time->cat[i] = initval[i];					      \
    }

  TESTARR_ELEM (abday, ( "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" ));
  TESTARR_ELEM (day, ( "Sunday", "Monday", "Tuesday", "Wednesday",
		        "Thursday", "Friday", "Saturday" ));
  TESTARR_ELEM (abmon, ( "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" ));
  TESTARR_ELEM (mon, ( "January", "February", "March", "April",
			"May", "June", "July", "August",
			"September", "October", "November", "December" ));
  TESTARR_ELEM (am_pm, ( "AM", "PM" ));

#define TEST_ELEM(cat, initval) \
  if (time->cat == NULL)						      \
    {									      \
      if (! be_quiet && ! nothing)					      \
	error (0, 0, _("%s: field `%s' not defined"), "LC_TIME", #cat);	      \
									      \
      time->cat = initval;						      \
    }

  TEST_ELEM (d_t_fmt, "%a %b %e %H:%M:%S %Y");
  TEST_ELEM (d_fmt, "%m/%d/%y");
  TEST_ELEM (t_fmt, "%H:%M:%S");

  /* According to C.Y.Alexis Cheng <alexis@vnet.ibm.com> the T_FMT_AMPM
     field is optional.  */
  if (time->t_fmt_ampm == NULL)
    {
      if (time->am_pm[0][0] == '\0' && time->am_pm[1][0] == '\0')
	{
	  /* No AM/PM strings defined, use the 24h format as default.  */
	  time->t_fmt_ampm = time->t_fmt;
	  time->wt_fmt_ampm = time->wt_fmt;
	}
      else
	{
	  time->t_fmt_ampm = "%I:%M:%S %p";
	  time->wt_fmt_ampm = (const uint32_t *) L"%I:%M:%S %p";
	}
    }

  /* Now process the era entries.  */
  if (time->num_era != 0)
    {
      const int days_per_month[12] = { 31, 29, 31, 30, 31, 30,
				       31, 31, 30, 31 ,30, 31 };
      size_t idx;
      wchar_t *wstr;

      time->era_entries =
	(struct era_data *) xmalloc (time->num_era
				     * sizeof (struct era_data));

      for (idx = 0; idx < time->num_era; ++idx)
	{
	  size_t era_len = strlen (time->era[idx]);
	  char *str = xmalloc ((era_len + 1 + 3) & ~3);
	  char *endp;

	  memcpy (str, time->era[idx], era_len + 1);

	  /* First character must be + or - for the direction.  */
	  if (*str != '+' && *str != '-')
	    {
	      if (!be_quiet)
		error (0, 0,
		       _("%s: direction flag in string %Zd in `era' field"
			 " is not '+' nor '-'"),
		       "LC_TIME", idx + 1);
	      /* Default arbitrarily to '+'.  */
	      time->era_entries[idx].direction = '+';
	    }
	  else
	    time->era_entries[idx].direction = *str;
	  if (*++str != ':')
	    {
	      if (!be_quiet)
		error (0, 0,
		       _("%s: direction flag in string %Zd in `era' field"
			 " is not a single character"),
		       "LC_TIME", idx + 1);
	      (void) strsep (&str, ":");
	    }
	  else
	    ++str;

	  /* Now the offset year.  */
	  time->era_entries[idx].offset = strtol (str, &endp, 10);
	  if (endp == str)
	    {
	      if (!be_quiet)
		error (0, 0, _("%s: invalid number for offset in string %Zd in"
			       " `era' field"),
		       "LC_TIME", idx + 1);
	      (void) strsep (&str, ":");
	    }
	  else if (*endp != ':')
	    {
	      if (!be_quiet)
		error (0, 0, _("%s: garbage at end of offset value in"
			       " string %Zd in `era' field"),
		       "LC_TIME", idx + 1);
	      (void) strsep (&str, ":");
	    }
	  else
	    str = endp + 1;

	  /* Next is the starting date in ISO format.  */
	  if (strncmp (str, "-*", 2) == 0)
	    {
	      time->era_entries[idx].start_date[0] =
		time->era_entries[idx].start_date[1] =
		time->era_entries[idx].start_date[2] = 0x80000000;
	      if (str[2] != ':')
		goto garbage_start_date;
	      str += 3;
	    }
	  else if (strncmp (str, "+*", 2) == 0)
	    {
	      time->era_entries[idx].start_date[0] =
		time->era_entries[idx].start_date[1] =
		time->era_entries[idx].start_date[2] = 0x7fffffff;
	      if (str[2] != ':')
		goto garbage_start_date;
	      str += 3;
	    }
	  else
	    {
	      time->era_entries[idx].start_date[0] = strtol (str, &endp, 10);
	      if (endp == str || *endp != '/')
		goto invalid_start_date;
	      else
		str = endp + 1;
	      time->era_entries[idx].start_date[0] -= 1900;
	      /* year -1 represent 1 B.C. (not -1 A.D.) */
	      if (time->era_entries[idx].start_date[0] < -1900)
		++time->era_entries[idx].start_date[0];

	      time->era_entries[idx].start_date[1] = strtol (str, &endp, 10);
	      if (endp == str || *endp != '/')
		goto invalid_start_date;
	      else
		str = endp + 1;
	      time->era_entries[idx].start_date[1] -= 1;

	      time->era_entries[idx].start_date[2] = strtol (str, &endp, 10);
	      if (endp == str)
		{
		invalid_start_date:
		  if (!be_quiet)
		    error (0, 0, _("%s: invalid starting date in string %Zd in"
				   " `era' field"),
			   "LC_TIME", idx + 1);
		  (void) strsep (&str, ":");
		}
	      else if (*endp != ':')
		{
		garbage_start_date:
		  if (!be_quiet)
		    error (0, 0, _("%s: garbage at end of starting date "
				   "in string %Zd in `era' field "),
			   "LC_TIME", idx + 1);
		  (void) strsep (&str, ":");
		}
	      else
		{
		  str = endp + 1;

		  /* Check for valid value.  */
		  if ((time->era_entries[idx].start_date[1] < 0
		       || time->era_entries[idx].start_date[1] >= 12
		       || time->era_entries[idx].start_date[2] < 0
		       || (time->era_entries[idx].start_date[2]
			   > days_per_month[time->era_entries[idx].start_date[1]])
		       || (time->era_entries[idx].start_date[1] == 2
			   && time->era_entries[idx].start_date[2] == 29
			   && !__isleap (time->era_entries[idx].start_date[0])))
		      && !be_quiet)
			  error (0, 0, _("%s: starting date is invalid in"
					 " string %Zd in `era' field"),
				 "LC_TIME", idx + 1);
		}
	    }

	  /* Next is the stopping date in ISO format.  */
	  if (strncmp (str, "-*", 2) == 0)
	    {
	      time->era_entries[idx].stop_date[0] =
		time->era_entries[idx].stop_date[1] =
		time->era_entries[idx].stop_date[2] = 0x80000000;
	      if (str[2] != ':')
		goto garbage_stop_date;
	      str += 3;
	    }
	  else if (strncmp (str, "+*", 2) == 0)
	    {
	      time->era_entries[idx].stop_date[0] =
		time->era_entries[idx].stop_date[1] =
		time->era_entries[idx].stop_date[2] = 0x7fffffff;
	      if (str[2] != ':')
		goto garbage_stop_date;
	      str += 3;
	    }
	  else
	    {
	      time->era_entries[idx].stop_date[0] = strtol (str, &endp, 10);
	      if (endp == str || *endp != '/')
		goto invalid_stop_date;
	      else
		str = endp + 1;
	      time->era_entries[idx].stop_date[0] -= 1900;
	      /* year -1 represent 1 B.C. (not -1 A.D.) */
	      if (time->era_entries[idx].stop_date[0] < -1900)
		++time->era_entries[idx].stop_date[0];

	      time->era_entries[idx].stop_date[1] = strtol (str, &endp, 10);
	      if (endp == str || *endp != '/')
		goto invalid_stop_date;
	      else
		str = endp + 1;
	      time->era_entries[idx].stop_date[1] -= 1;

	      time->era_entries[idx].stop_date[2] = strtol (str, &endp, 10);
	      if (endp == str)
		{
		invalid_stop_date:
		  if (!be_quiet)
		    error (0, 0, _("%s: invalid stopping date in string %Zd in"
				   " `era' field"),
			   "LC_TIME", idx + 1);
		  (void) strsep (&str, ":");
		}
	      else if (*endp != ':')
		{
		garbage_stop_date:
		  if (!be_quiet)
		    error (0, 0, _("%s: garbage at end of stopping date "
				   "in string %Zd in `era' field"),
			   "LC_TIME", idx + 1);
		  (void) strsep (&str, ":");
		}
	      else
		{
		  str = endp + 1;

		  /* Check for valid value.  */
		  if ((time->era_entries[idx].stop_date[1] < 0
		       || time->era_entries[idx].stop_date[1] >= 12
		       || time->era_entries[idx].stop_date[2] < 0
		       || (time->era_entries[idx].stop_date[2]
			   > days_per_month[time->era_entries[idx].stop_date[1]])
		       || (time->era_entries[idx].stop_date[1] == 2
			   && time->era_entries[idx].stop_date[2] == 29
			   && !__isleap (time->era_entries[idx].stop_date[0])))
		      && !be_quiet)
			  error (0, 0, _("%s: stopping date is invalid in"
					 " string %Zd in `era' field"),
				 "LC_TIME", idx + 1);
		}
	    }

	  if (str == NULL || *str == '\0')
	    {
	      if (!be_quiet)
		error (0, 0, _("%s: missing era name in string %Zd in `era'"
			       " field"), "LC_TIME", idx + 1);
	      time->era_entries[idx].name =
		time->era_entries[idx].format = "";
	    }
	  else
	    {
	      time->era_entries[idx].name = strsep (&str, ":");

	      if (str == NULL || *str == '\0')
		{
		  if (!be_quiet)
		    error (0, 0, _("%s: missing era format in string %Zd"
				   " in `era' field"),
			   "LC_TIME", idx + 1);
		  time->era_entries[idx].name =
		    time->era_entries[idx].format = "";
		}
	      else
		time->era_entries[idx].format = str;
	    }

	  /* Now generate the wide character name and format.  */
	  wstr = wcschr ((wchar_t *) time->wera[idx], L':');/* end direction */
	  wstr = wstr ? wcschr (wstr + 1, L':') : NULL;	/* end offset */
	  wstr = wstr ? wcschr (wstr + 1, L':') : NULL;	/* end start */
	  wstr = wstr ? wcschr (wstr + 1, L':') : NULL;	/* end end */
	  time->era_entries[idx].wname = (uint32_t *) wstr + 1;
	  wstr = wstr ? wcschr (wstr + 1, L':') : NULL;	/* end name */
	  *wstr = L'\0';
	  time->era_entries[idx].wformat = (uint32_t *) wstr + 1;
	}
    }

  if (time->week_ndays == 0)
    time->week_ndays = 7;

  if (time->week_1stday == 0)
    time->week_1stday = 19971130;

  if (time->week_1stweek > time->week_ndays)
    error (0, 0, _("\
%s: third operand for value of field `%s' must not be larger than %d"),
	   "LC_TIME", "week", 7);

  if (time->first_weekday == '\0')
    /* The definition does not specify this so the default is used.  */
    time->first_weekday = 1;
  else if (time->first_weekday > time->week_ndays)
    error (0, 0, _("\
%s: values of field `%s' must not be larger than %d"),
	   "LC_TIME", "first_weekday", 7);

  if (time->first_workday == '\0')
    /* The definition does not specify this so the default is used.  */
    time->first_workday = 1;
  else if (time->first_workday > time->week_ndays)
    error (0, 0, _("\
%s: values of field `%s' must not be larger than %d"),
	   "LC_TIME", "first_workday", 7);

  if (time->cal_direction == '\0')
    /* The definition does not specify this so the default is used.  */
    time->cal_direction = 1;
  else if (time->cal_direction > 3)
    error (0, 0, _("\
%s: values for field `%s' must not be larger than %d"),
	   "LC_TIME", "cal_direction", 3);

  /* XXX We don't perform any tests on the timezone value since this is
     simply useless, stupid $&$!@...  */
  if (time->timezone == NULL)
    time->timezone = "";

  if (time->date_fmt == NULL)
    time->date_fmt = "%a %b %e %H:%M:%S %Z %Y";
  if (time->wdate_fmt == NULL)
    time->wdate_fmt = (const uint32_t *) L"%a %b %e %H:%M:%S %Z %Y";
}


void
time_output (struct localedef_t *locale, struct charmap_t *charmap,
	     const char *output_path)
{
  struct locale_time_t *time = locale->categories[LC_TIME].time;
  struct iovec iov[2 + _NL_ITEM_INDEX (_NL_NUM_LC_TIME)
		  + time->num_era - 1
		  + 2 * 99
		  + 2 + time->num_era * 10 - 1];
  struct locale_file data;
  uint32_t idx[_NL_ITEM_INDEX (_NL_NUM_LC_TIME)];
  size_t cnt, last_idx, num, n;

  data.magic = LIMAGIC (LC_TIME);
  data.n = _NL_ITEM_INDEX (_NL_NUM_LC_TIME);
  iov[0].iov_base = (void *) &data;
  iov[0].iov_len = sizeof (data);

  iov[1].iov_base = (void *) idx;
  iov[1].iov_len = sizeof (idx);

  idx[0] = iov[0].iov_len + iov[1].iov_len;

  /* The ab'days.  */
  for (cnt = 0; cnt <= _NL_ITEM_INDEX (ABDAY_7); ++cnt)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->abday[cnt - _NL_ITEM_INDEX (ABDAY_1)] ?: "");
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
    }

  /* The days.  */
  for (; cnt <= _NL_ITEM_INDEX (DAY_7); ++cnt)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->day[cnt - _NL_ITEM_INDEX (DAY_1)] ?: "");
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
    }

  /* The ab'mons.  */
  for (; cnt <= _NL_ITEM_INDEX (ABMON_12); ++cnt)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->abmon[cnt - _NL_ITEM_INDEX (ABMON_1)] ?: "");
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
    }

  /* The mons.  */
  for (; cnt <= _NL_ITEM_INDEX (MON_12); ++cnt)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->mon[cnt - _NL_ITEM_INDEX (MON_1)] ?: "");
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
    }

  /* AM/PM.  */
  for (; cnt <= _NL_ITEM_INDEX (PM_STR); ++cnt)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->am_pm[cnt - _NL_ITEM_INDEX (AM_STR)] ?: "");
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
    }

  iov[2 + cnt].iov_base = (void *) (time->d_t_fmt ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
  ++cnt;

  iov[2 + cnt].iov_base = (void *) (time->d_fmt ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
  ++cnt;

  iov[2 + cnt].iov_base = (void *) (time->t_fmt ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
  ++cnt;

  iov[2 + cnt].iov_base = (void *) (time->t_fmt_ampm ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + cnt] = idx[cnt] + iov[2 + cnt].iov_len;
  last_idx = ++cnt;

  idx[1 + last_idx] = idx[last_idx];
  for (num = 0; num < time->num_era; ++num, ++cnt)
    {
      iov[2 + cnt].iov_base = (void *) time->era[num];
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + last_idx] += iov[2 + cnt].iov_len;
    }
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->era_year ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->era_d_fmt ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  idx[1 + last_idx] = idx[last_idx];
  for (num = 0; num < 100; ++num, ++cnt)
    {
      iov[2 + cnt].iov_base = (void *) (time->alt_digits[num] ?: "");
      iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
      idx[1 + last_idx] += iov[2 + cnt].iov_len;
    }
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->era_d_t_fmt ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->era_t_fmt ?: "");
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;


  /* We must align the following data.  */
  iov[2 + cnt].iov_base = (void *) "\0\0";
  iov[2 + cnt].iov_len = ((idx[last_idx] + 3) & ~3) - idx[last_idx];
  idx[last_idx] = (idx[last_idx] + 3) & ~3;
  ++cnt;

  /* The `era' data in usable form.  */
  iov[2 + cnt].iov_base = (void *) &time->num_era;
  iov[2 + cnt].iov_len = sizeof (uint32_t);
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  idx[1 + last_idx] = idx[last_idx];
  for (num = 0; num < time->num_era; ++num)
    {
      size_t l;

      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].direction;
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].offset;
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].start_date[0];
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].start_date[1];
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].start_date[2];
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].stop_date[0];
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].stop_date[1];
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;
      iov[2 + cnt].iov_base = (void *) &time->era_entries[num].stop_date[2];
      iov[2 + cnt].iov_len = sizeof (int32_t);
      ++cnt;

      l = (strchr (time->era_entries[num].format, '\0')
	   - time->era_entries[num].name) + 1;
      l = (l + 3) & ~3;
      iov[2 + cnt].iov_base = (void *) time->era_entries[num].name;
      iov[2 + cnt].iov_len = l;
      ++cnt;

      idx[1 + last_idx] += 8 * sizeof (int32_t) + l;

      assert (idx[1 + last_idx] % 4 == 0);

      iov[2 + cnt].iov_base = (void *) time->era_entries[num].wname;
      iov[2 + cnt].iov_len = ((wcschr ((wchar_t *) time->era_entries[num].wformat, L'\0')
			       - (wchar_t *) time->era_entries[num].wname + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] += iov[2 + cnt].iov_len;
      ++cnt;
    }
  ++last_idx;

  /* The wide character ab'days.  */
  for (n = 0; n < 7; ++n, ++cnt, ++last_idx)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->wabday[n] ?: empty_wstr);
      iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
    }

  /* The wide character days.  */
  for (n = 0; n < 7; ++n, ++cnt, ++last_idx)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->wday[n] ?: empty_wstr);
      iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
    }

  /* The wide character ab'mons.  */
  for (n = 0; n < 12; ++n, ++cnt, ++last_idx)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->wabmon[n] ?: empty_wstr);
      iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
    }

  /* The wide character mons.  */
  for (n = 0; n < 12; ++n, ++cnt, ++last_idx)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->wmon[n] ?: empty_wstr);
      iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
    }

  /* Wide character AM/PM.  */
  for (n = 0; n < 2; ++n, ++cnt, ++last_idx)
    {
      iov[2 + cnt].iov_base =
	(void *) (time->wam_pm[n] ?: empty_wstr);
      iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
    }

  iov[2 + cnt].iov_base = (void *) (time->wd_t_fmt ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wd_fmt ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wt_fmt ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wt_fmt_ampm ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wera_year ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wera_d_fmt ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  idx[1 + last_idx] = idx[last_idx];
  for (num = 0; num < 100; ++num, ++cnt)
    {
      iov[2 + cnt].iov_base = (void *) (time->walt_digits[num]
					?: empty_wstr);
      iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			      * sizeof (uint32_t));
      idx[1 + last_idx] += iov[2 + cnt].iov_len;
    }
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wera_d_t_fmt ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) (time->wera_t_fmt ?: empty_wstr);
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
			  * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) &time->week_ndays;
  iov[2 + cnt].iov_len = 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  /* We must align the following data.  */
  iov[2 + cnt].iov_base = (void *) "\0\0";
  iov[2 + cnt].iov_len = ((idx[last_idx] + 3) & ~3) - idx[last_idx];
  idx[last_idx] = (idx[last_idx] + 3) & ~3;
  ++cnt;

  iov[2 + cnt].iov_base = (void *) &time->week_1stday;
  iov[2 + cnt].iov_len = sizeof(uint32_t);
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) &time->week_1stweek;
  iov[2 + cnt].iov_len = 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) &time->first_weekday;
  iov[2 + cnt].iov_len = 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) &time->first_workday;
  iov[2 + cnt].iov_len = 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) &time->cal_direction;
  iov[2 + cnt].iov_len = 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) time->timezone;
  iov[2 + cnt].iov_len = strlen (time->timezone) + 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) time->date_fmt;
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) time->wdate_fmt;
  iov[2 + cnt].iov_len = ((wcslen (iov[2 + cnt].iov_base) + 1)
                          * sizeof (uint32_t));
  idx[1 + last_idx] = idx[last_idx] + iov[2 + cnt].iov_len;
  ++cnt;
  ++last_idx;

  iov[2 + cnt].iov_base = (void *) charmap->code_set_name;
  iov[2 + cnt].iov_len = strlen (iov[2 + cnt].iov_base) + 1;
  ++cnt;
  ++last_idx;

  assert (cnt == (_NL_ITEM_INDEX (_NL_NUM_LC_TIME)
		  + time->num_era - 1
		  + 2 * 99
		  + 2 + time->num_era * 10 - 1));
  assert (last_idx  == _NL_ITEM_INDEX (_NL_NUM_LC_TIME));

  write_locale_data (output_path, "LC_TIME", 2 + cnt, iov);
}


/* The parser for the LC_TIME section of the locale definition.  */
void
time_read (struct linereader *ldfile, struct localedef_t *result,
	   struct charmap_t *charmap, const char *repertoire_name,
	   int ignore_content)
{
  struct repertoire_t *repertoire = NULL;
  struct locale_time_t *time;
  struct token *now;
  enum token_t nowtok;
  size_t cnt;

  /* Get the repertoire we have to use.  */
  if (repertoire_name != NULL)
    repertoire = repertoire_read (repertoire_name);

  /* The rest of the line containing `LC_TIME' must be free.  */
  lr_ignore_rest (ldfile, 1);


  do
    {
      now = lr_token (ldfile, charmap, repertoire, verbose);
      nowtok = now->tok;
    }
  while (nowtok == tok_eol);

  /* If we see `copy' now we are almost done.  */
  if (nowtok == tok_copy)
    {
      handle_copy (ldfile, charmap, repertoire_name, result, tok_lc_time,
		   LC_TIME, "LC_TIME", ignore_content);
      return;
    }

  /* Prepare the data structures.  */
  time_startup (ldfile, result, ignore_content);
  time = result->categories[LC_TIME].time;

  while (1)
    {
      /* Of course we don't proceed beyond the end of file.  */
      if (nowtok == tok_eof)
	break;

      /* Ingore empty lines.  */
      if (nowtok == tok_eol)
	{
	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  nowtok = now->tok;
	  continue;
	}

      switch (nowtok)
	{
#define STRARR_ELEM(cat, min, max) \
	case tok_##cat:							      \
	  /* Ignore the rest of the line if we don't need the input of	      \
	     this line.  */						      \
	  if (ignore_content)						      \
	    {								      \
	      lr_ignore_rest (ldfile, 0);				      \
	      break;							      \
	    }								      \
									      \
	  for (cnt = 0; cnt < max; ++cnt)				      \
	    {								      \
	      now = lr_token (ldfile, charmap, repertoire, verbose);	      \
	      if (now->tok == tok_eol)					      \
		{							      \
		  if (cnt < min)					      \
		    lr_error (ldfile, _("%s: too few values for field `%s'"), \
			      "LC_TIME", #cat);				      \
		  if (!ignore_content)					      \
		    do							      \
		      {							      \
			time->cat[cnt] = "";				      \
			time->w##cat[cnt] = empty_wstr;			      \
		      }							      \
		    while (++cnt < max);				      \
		  break;						      \
		}							      \
	      else if (now->tok != tok_string)				      \
		goto err_label;						      \
	      else if (!ignore_content && (now->val.str.startmb == NULL	      \
					   || now->val.str.startwc == NULL))  \
		{							      \
		  lr_error (ldfile, _("%s: unknown character in field `%s'"), \
			    "LC_TIME", #cat);				      \
		  time->cat[cnt] = "";					      \
		  time->w##cat[cnt] = empty_wstr;			      \
		}							      \
	      else if (!ignore_content)					      \
		{							      \
		  time->cat[cnt] = now->val.str.startmb;		      \
		  time->w##cat[cnt] = now->val.str.startwc;		      \
		}							      \
									      \
	      /* Match the semicolon.  */				      \
	      now = lr_token (ldfile, charmap, repertoire, verbose);	      \
	      if (now->tok != tok_semicolon && now->tok != tok_eol)	      \
		break;							      \
	    }								      \
	  if (now->tok != tok_eol)					      \
	    {								      \
	      while (!ignore_content && cnt < min)			      \
		{							      \
		  time->cat[cnt] = "";					      \
		  time->w##cat[cnt++] = empty_wstr;			      \
		}							      \
	      								      \
	      if (now->tok == tok_semicolon)				      \
		{							      \
		  now = lr_token (ldfile, charmap, repertoire, verbose);      \
		  if (now->tok == tok_eol)				      \
		    lr_error (ldfile, _("extra trailing semicolon"));	      \
		  else if (now->tok == tok_string)			      \
		    {							      \
		      lr_error (ldfile, _("\
%s: too many values for field `%s'"),					      \
				"LC_TIME", #cat);			      \
		      lr_ignore_rest (ldfile, 0);			      \
		    }							      \
		  else							      \
		    goto err_label;					      \
		}							      \
	      else							      \
		goto err_label;						      \
	    }								      \
	  time->cat##_defined = 1;					      \
	  break

	  STRARR_ELEM (abday, 7, 7);
	  STRARR_ELEM (day, 7, 7);
	  STRARR_ELEM (abmon, 12, 12);
	  STRARR_ELEM (mon, 12, 12);
	  STRARR_ELEM (am_pm, 2, 2);
	  STRARR_ELEM (alt_digits, 0, 100);

	case tok_era:
	  /* Ignore the rest of the line if we don't need the input of
	     this line.  */
	  if (ignore_content)
	    {
	      lr_ignore_rest (ldfile, 0);
	      break;
	    }
	  do
	    {
	      now = lr_token (ldfile, charmap, repertoire, verbose);
	      if (now->tok != tok_string)
		goto err_label;
	      if (!ignore_content && (now->val.str.startmb == NULL
				      || now->val.str.startwc == NULL))
		{
		  lr_error (ldfile, _("%s: unknown character in field `%s'"),
			    "LC_TIME", "era");
		  lr_ignore_rest (ldfile, 0);
		  break;
		}
	      if (!ignore_content)
		{
		  time->era = xrealloc (time->era,
					(time->num_era + 1) * sizeof (char *));
		  time->era[time->num_era] = now->val.str.startmb;

		  time->wera = xrealloc (time->wera,
					 (time->num_era + 1)
					 * sizeof (char *));
		  time->wera[time->num_era++] = now->val.str.startwc;
		}
	      now = lr_token (ldfile, charmap, repertoire, verbose);
	      if (now->tok != tok_eol && now->tok != tok_semicolon)
		goto err_label;
	    }
	  while (now->tok == tok_semicolon);
	  break;

#define STR_ELEM(cat) \
	case tok_##cat:							      \
	  /* Ignore the rest of the line if we don't need the input of	      \
	     this line.  */						      \
	  if (ignore_content)						      \
	    {								      \
	      lr_ignore_rest (ldfile, 0);				      \
	      break;							      \
	    }								      \
									      \
	  now = lr_token (ldfile, charmap, repertoire, verbose);	      \
	  if (now->tok != tok_string)					      \
	    goto err_label;						      \
	  else if (time->cat != NULL)					      \
	    lr_error (ldfile, _("\
%s: field `%s' declared more than once"), "LC_TIME", #cat);		      \
	  else if (!ignore_content && (now->val.str.startmb == NULL	      \
				       || now->val.str.startwc == NULL))      \
	    {								      \
	      lr_error (ldfile, _("%s: unknown character in field `%s'"),     \
			"LC_TIME", #cat);				      \
	      time->cat = "";						      \
	      time->w##cat = empty_wstr;				      \
	    }								      \
	  else if (!ignore_content)					      \
	    {								      \
	      time->cat = now->val.str.startmb;				      \
	      time->w##cat = now->val.str.startwc;			      \
	    }								      \
	  break

	  STR_ELEM (d_t_fmt);
	  STR_ELEM (d_fmt);
	  STR_ELEM (t_fmt);
	  STR_ELEM (t_fmt_ampm);
	  STR_ELEM (era_year);
	  STR_ELEM (era_d_t_fmt);
	  STR_ELEM (era_d_fmt);
	  STR_ELEM (era_t_fmt);
	  STR_ELEM (timezone);
	  STR_ELEM (date_fmt);

#define INT_ELEM(cat) \
	case tok_##cat:							      \
	  /* Ignore the rest of the line if we don't need the input of	      \
	     this line.  */						      \
	  if (ignore_content)						      \
	    {								      \
	      lr_ignore_rest (ldfile, 0);				      \
	      break;							      \
	    }								      \
									      \
	  now = lr_token (ldfile, charmap, repertoire, verbose);	      \
	  if (now->tok != tok_number)					      \
	    goto err_label;						      \
	  else if (time->cat != 0)					      \
	    lr_error (ldfile, _("%s: field `%s' declared more than once"),    \
		      "LC_TIME", #cat);					      \
	  else if (!ignore_content)					      \
	    time->cat = now->val.num;					      \
	  break

	  INT_ELEM (first_weekday);
	  INT_ELEM (first_workday);
	  INT_ELEM (cal_direction);

	case tok_week:
	  /* Ignore the rest of the line if we don't need the input of
	     this line.  */
	  if (ignore_content)
	    {
	      lr_ignore_rest (ldfile, 0);
	      break;
	    }

	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  if (now->tok != tok_number)
	    goto err_label;
	  time->week_ndays = now->val.num;

	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  if (now->tok != tok_semicolon)
	    goto err_label;

	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  if (now->tok != tok_number)
	    goto err_label;
	  time->week_1stday = now->val.num;

	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  if (now->tok != tok_semicolon)
	    goto err_label;

	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  if (now->tok != tok_number)
	    goto err_label;
	  time->week_1stweek = now->val.num;

	  lr_ignore_rest (ldfile,  1);
	  break;

	case tok_end:
	  /* Next we assume `LC_TIME'.  */
	  now = lr_token (ldfile, charmap, repertoire, verbose);
	  if (now->tok == tok_eof)
	    break;
	  if (now->tok == tok_eol)
	    lr_error (ldfile, _("%s: incomplete `END' line"), "LC_TIME");
	  else if (now->tok != tok_lc_time)
	    lr_error (ldfile, _("\
%1$s: definition does not end with `END %1$s'"), "LC_TIME");
	  lr_ignore_rest (ldfile, now->tok == tok_lc_time);
	  return;

	default:
	err_label:
	  SYNTAX_ERROR (_("%s: syntax error"), "LC_TIME");
	}

      /* Prepare for the next round.  */
      now = lr_token (ldfile, charmap, repertoire, verbose);
      nowtok = now->tok;
    }

  /* When we come here we reached the end of the file.  */
  lr_error (ldfile, _("%s: premature end of file"), "LC_TIME");
}
