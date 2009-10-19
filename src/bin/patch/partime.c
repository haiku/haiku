/* Parse a string, yielding a struct partime that describes it.  */

/* Copyright 1993, 1994, 1995, 1997 Paul Eggert
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
# if HAVE_LIMITS_H
#  include <limits.h>
# endif
# ifndef LONG_MIN
# define LONG_MIN (-1-2147483647L)
# endif
# if HAVE_STDDEF_H
#  include <stddef.h>
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

#ifndef offsetof
#define offsetof(aggregate, member) ((size_t) &((aggregate *) 0)->member)
#endif

#include <ctype.h>
#if STDC_HEADERS
# define CTYPE_DOMAIN(c) 1
#else
# define CTYPE_DOMAIN(c) ((unsigned) (c) <= 0177)
#endif
#define ISALNUM(c)	(CTYPE_DOMAIN (c) && isalnum (c))
#define ISALPHA(c)	(CTYPE_DOMAIN (c) && isalpha (c))
#define ISSPACE(c)	(CTYPE_DOMAIN (c) && isspace (c))
#define ISUPPER(c)	(CTYPE_DOMAIN (c) && isupper (c))
#define ISDIGIT(c)	((unsigned) (c) - '0' <= 9)

#include <partime.h>

char const partime_id[] =
  "$Id: partime.c 8008 2004-06-16 21:22:10Z korli $";


/* Lookup tables for names of months, weekdays, time zones.  */

#define NAME_LENGTH_MAXIMUM 4

struct name_val
  {
    char name[NAME_LENGTH_MAXIMUM];
    int val;
  };


static char const *parse_decimal P ((char const *, int, int, int, int, int *, int *));
static char const *parse_fixed P ((char const *, int, int *));
static char const *parse_pattern_letter P ((char const *, int, struct partime *));
static char const *parse_prefix P ((char const *, char const **, struct partime *));
static char const *parse_ranged P ((char const *, int, int, int, int *));
static char const *parse_varying P ((char const *, int *));
static int lookup P ((char const *, struct name_val const[]));
static int merge_partime P ((struct partime *, struct partime const *));
static void undefine P ((struct partime *));


static struct name_val const month_names[] =
{
  {"jan", 0},
  {"feb", 1},
  {"mar", 2},
  {"apr", 3},
  {"may", 4},
  {"jun", 5},
  {"jul", 6},
  {"aug", 7},
  {"sep", 8},
  {"oct", 9},
  {"nov", 10},
  {"dec", 11},
  {"", TM_UNDEFINED}
};

static struct name_val const weekday_names[] =
{
  {"sun", 0},
  {"mon", 1},
  {"tue", 2},
  {"wed", 3},
  {"thu", 4},
  {"fri", 5},
  {"sat", 6},
  {"", TM_UNDEFINED}
};

#define RELATIVE_CONS(member, multiplier)	\
	(offsetof (struct tm, member) + (multiplier) * sizeof (struct tm))
#define RELATIVE_OFFSET(c)	((c) % sizeof (struct tm))
#define RELATIVE_MULTIPLIER(c)	((c) / sizeof (struct tm))
static struct name_val const relative_units[] =
{
  {"year", RELATIVE_CONS (tm_year,  1) },
  {"mont", RELATIVE_CONS (tm_mon ,  1) },
  {"fort", RELATIVE_CONS (tm_mday, 14) },
  {"week", RELATIVE_CONS (tm_mday,  7) },
  {"day" , RELATIVE_CONS (tm_mday,  1) },
  {"hour", RELATIVE_CONS (tm_hour,  1) },
  {"min" , RELATIVE_CONS (tm_min ,  1) },
  {"sec" , RELATIVE_CONS (tm_sec ,  1) },
  {"", TM_UNDEFINED}
};

static struct name_val const ago[] =
{
  {"ago", 0},
  {"", TM_UNDEFINED}
};

static struct name_val const dst_names[] =
{
  {"dst", 1},
  {"", 0}
};

#define hr60nonnegative(t)	((t)/100 * 60  +  (t)%100)
#define hr60(t)	((t) < 0 ? - hr60nonnegative (-(t)) : hr60nonnegative (t))
#define zs(t, s)	{s, hr60 (t)}
#define zd(t, s, d)	zs (t, s),  zs ((t) + 100, d)

static struct name_val const zone_names[] =
{
  zs (-1000, "hst"),		/* Hawaii */
  zd (-1000, "hast", "hadt"),	/* Hawaii-Aleutian */
  zd (- 900, "akst", "akdt"),	/* Alaska */
  zd (- 800, "pst" , "pdt" ),	/* Pacific */
  zd (- 700, "mst" , "mdt" ),	/* Mountain */
  zd (- 600, "cst" , "cdt" ),	/* Central */
  zd (- 500, "est" , "edt" ),	/* Eastern */
  zd (- 400, "ast" , "adt" ),	/* Atlantic */
  zd (- 330, "nst" , "ndt" ),	/* Newfoundland */
  zs (  000, "utc" ),		/* Coordinated Universal */
  zs (  000, "uct" ),		/* " */
  zs (  000, "cut" ),		/* " */
  zs (  000, "ut"),		/* Universal */
  zs (  000, "z"),		/* Zulu (required by ISO 8601) */
  zd (  000, "gmt" , "bst" ),	/* Greenwich Mean, British Summer */
  zd (  000, "wet" , "west"),	/* Western European */
  zd (  100, "cet" , "cest"),	/* Central European */
  zd (  100, "met" , "mest"),	/* Middle European (bug in old tz versions) */
  zd (  100, "mez" , "mesz"),	/* Mittel-Europaeische Zeit */
  zd (  200, "eet" , "eest"),	/* Eastern European */
  zs (  530, "ist" ),		/* India */
  zd (  900, "jst" , "jdt" ),	/* Japan */
  zd (  900, "kst" , "kdt" ),	/* Korea */
  zd ( 1200, "nzst", "nzdt"),	/* New Zealand */
  {"lt", 1},
#if 0
  /* The following names are duplicates or are not well attested.
     It's not worth keeping a complete list, since alphabetic time zone names
     are deprecated and there are lots more where these came from.  */
  zs (-1100, "sst" ),		/* Samoan */
  zd (- 900, "yst" , "ydt" ),	/* Yukon - name is no longer used */
  zd (- 500, "ast" , "adt" ),	/* Acre */
  zd (- 400, "wst" , "wdt" ),	/* Western Brazil */
  zd (- 400, "cst" , "cdt" ),	/* Chile */
  zd (- 200, "fst" , "fdt" ),	/* Fernando de Noronha */
  zs (  000, "wat" ),		/* West African */
  zs (  100, "cat" ),		/* Central African */
  zs (  200, "sat" ),		/* South African */
  zd (  200, "ist" , "idt" ),	/* Israel */
  zs (  300, "eat" ),		/* East African */
  zd (  300, "msk" , "msd" ),	/* Moscow */
  zd (  330, "ist" , "idt" ),	/* Iran */
  zs (  800, "hkt" ),		/* Hong Kong */
  zs (  800, "sgt" ),		/* Singapore */
  zd (  800, "cst" , "cdt" ),	/* China */
  zd (  800, "wst" , "wst" ),	/* Western Australia */
  zd (  930, "cst" , "cst" ),	/* Central Australia */
  zs ( 1000, "gst" ),		/* Guam */
  zd ( 1000, "est" , "est" ),	/* Eastern Australia */
#endif
  {"", -1}
};

/* Look for a prefix of S in TABLE, returning val for first matching entry.  */
static int
lookup (s, table)
     char const *s;
     struct name_val const table[];
{
  int j;
  char buf[NAME_LENGTH_MAXIMUM];

  for (j = 0; j < NAME_LENGTH_MAXIMUM; j++)
    {
      unsigned char c = *s;
      if (! ISALPHA (c))
	{
	  buf[j] = '\0';
	  break;
	}
      buf[j] = ISUPPER (c) ? tolower (c) : c;
      s++;
      s += *s == '.';
    }

  for (;; table++)
    for (j = 0; ; j++)
      if (j == NAME_LENGTH_MAXIMUM  ||  ! table[0].name[j])
	return table[0].val;
      else if (buf[j] != table[0].name[j])
	break;
}


/* Set *T to ``undefined'' values.  */
static void
undefine (t)
     struct partime *t;
{
  t->tm.tm_sec = t->tm.tm_min = t->tm.tm_hour = t->tm.tm_mday = t->tm.tm_mon
    = t->tm.tm_year = t->tm.tm_wday = t->tm.tm_yday
    = t->wday_ordinal = t->ymodulus = t->yweek
    = TM_UNDEFINED;
  t->tmr.tm_sec = t->tmr.tm_min = t->tmr.tm_hour =
    t->tmr.tm_mday = t->tmr.tm_mon = t->tmr.tm_year = 0;
  t->zone = TM_UNDEFINED_ZONE;
}

/* Patterns to look for in a time string.
   Order is important: we look for the first matching pattern
   whose values do not contradict values that we already know about.
   See `parse_pattern_letter' below for the meaning of the pattern codes.  */
static char const time_patterns[] =
{
  /* Traditional patterns come first,
     to prevent an ISO 8601 format from misinterpreting their prefixes.  */

  /* RFC 822, extended */
  'E', '_', 'N', '_', 'y', '$', 0,
  'x', 0,

  /* traditional */
  '4', '_', 'M', '_', 'D', '_', 'h', '_', 'm', '_', 's', '$', 0,
  'R', '_', 'M', '_', 'D', '_', 'h', '_', 'm', '_', 's', '$', 0,
  'E', '_', 'N', 0,
  'N', '_', 'E', '_', 'y', ';', 0,
  'N', '_', 'E', ';', 0,
  'N', 0,
  't', ':', 'm', ':', 's', '_', 'A', 0,
  't', ':', 'm', '_', 'A', 0,
  't', '_', 'A', 0,

  /* traditional get_date */
  'i', '_', 'x', 0,
  'Y', '/', 'n', '/', 'E', ';', 0,
  'n', '/', 'E', '/', 'y', ';', 0,
  'n', '/', 'E', ';', 0,
  'u', 0,

  /* ISO 8601:1988 formats, generalized a bit.  */
  'y', '-', 'M', '-', 'D', '$', 0,
  '4', 'M', 'D', '$', 0,
  'Y', '-', 'M', '$', 0,
  'R', 'M', 'D', '$', 0,
  '-', 'R', '=', 'M', '$', 0,
  '-', 'R', '$', 0,
  '-', '-', 'M', '=', 'D', '$', 0,
  'M', '=', 'D', 'T', 0,
  '-', '-', 'M', '$', 0,
  '-', '-', '-', 'D', '$', 0,
  'D', 'T', 0,
  'Y', '-', 'd', '$', 0,
  '4', 'd', '$', 0,
  'R', '=', 'd', '$', 0,
  '-', 'd', '$', 0,
  'd', 'T', 0,
  'y', '-', 'W', '-', 'X', 0,
  'y', 'W', 'X', 0,
  'y', '=', 'W', 0,
  '-', 'r', '-', 'W', '-', 'X', 0,
  'r', '-', 'W', '-', 'X', 'T', 0,
  '-', 'r', 'W', 'X', 0,
  'r', 'W', 'X', 'T', 0,
  '-', 'W', '=', 'X', 0,
  'W', '=', 'X', 'T', 0,
  '-', 'W', 0,
  '-', 'w', '-', 'X', 0,
  'w', '-', 'X', 'T', 0,
  '-', '-', '-', 'X', '$', 0,
  'X', 'T', 0,
  '4', '$', 0,
  'T', 0,
  'h', ':', 'm', ':', 's', '$', 0,
  'h', 'm', 's', '$', 0,
  'h', ':', 'L', '$', 0,
  'h', 'L', '$', 0,
  'H', '$', 0,
  '-', 'm', ':', 's', '$', 0,
  '-', 'm', 's', '$', 0,
  '-', 'L', '$', 0,
  '-', '-', 's', '$', 0,
  'Y', 0,
  'Z', 0,

  0
};

/* Parse an initial prefix of STR according to *PATTERNS, setting *T.
   Return the first character after the prefix, or 0 if it couldn't be parsed.
   *PATTERNS is a character array containing one pattern string after another;
   it is terminated by an empty string.
   If success, set *PATTERNS to the next pattern to try.
   Set *PATTERNS to 0 if we know there are no more patterns to try;
   if *PATTERNS is initially 0, give up immediately.  */
static char const *
parse_prefix (str, patterns, t)
     char const *str;
     char const **patterns;
     struct partime *t;
{
  char const *pat = *patterns;
  unsigned char c;

  if (! pat)
    return 0;

  /* Remove initial noise.  */
  while (! ISALNUM (c = *str) && c != '-' && c != '+')
    {
      if (! c)
	{
	  undefine (t);
	  *patterns = 0;
	  return str;
	}

      str++;
    }

  /* Try a pattern until one succeeds.  */
  while (*pat)
    {
      char const *s = str;
      undefine (t);

      do
	{
	  if (! (c = *pat++))
	    {
	      *patterns = pat;
	      return s;
	    }
	}
      while ((s = parse_pattern_letter (s, c, t)) != 0);

      while (*pat++)
	continue;
    }

  return 0;
}

/* Parse an initial prefix of S of length DIGITS; it must be a number.
   Store the parsed number into *RES.
   Return the first character after the prefix, or 0 if it wasn't parsed.  */
static char const *
parse_fixed (s, digits, res)
     char const *s;
     int digits, *res;
{
  int n = 0;
  char const *lim = s + digits;
  while (s < lim)
    {
      unsigned d = *s++ - '0';
      if (9 < d)
	return 0;
      n = 10 * n + d;
    }
  *res = n;
  return s;
}

/* Parse a possibly empty initial prefix of S.
   Store the parsed number into *RES.
   Return the first character after the prefix.  */
static char const *
parse_varying (s, res)
     char const *s;
     int *res;
{
  int n = 0;
  for (;;)
    {
      unsigned d = *s - '0';
      if (9 < d)
	break;
      s++;
      n = 10 * n + d;
    }
  *res = n;
  return s;
}

/* Parse an initial prefix of S of length DIGITS;
   it must be a number in the range LO through HI.
   Store the parsed number into *RES.
   Return the first character after the prefix, or 0 if it wasn't parsed.  */
static char const *
parse_ranged (s, digits, lo, hi, res)
     char const *s;
     int digits, lo, hi, *res;
{
  s = parse_fixed (s, digits, res);
  return s && lo <= *res && *res <= hi ? s : 0;
}

/* Parse an initial prefix of S of length DIGITS;
   it must be a number in the range LO through HI
   and it may be followed by a fraction to be computed using RESOLUTION.
   Store the parsed number into *RES; store the fraction times RESOLUTION,
   rounded to the nearest integer, into *FRES.
   Return the first character after the prefix, or 0 if it wasn't parsed.  */
static char const *
parse_decimal (s, digits, lo, hi, resolution, res, fres)
     char const *s;
     int digits, lo, hi, resolution, *res, *fres;
{
  s = parse_fixed (s, digits, res);
  if (s && lo <= *res && *res <= hi)
    {
      int f = 0;
      if ((s[0] == ',' || s[0] == '.') && ISDIGIT (s[1]))
	{
	  char const *s1 = ++s;
	  int num10 = 0, denom10 = 10, product;
	  while (ISDIGIT (*++s))
	    {
	      int d = denom10 * 10;
	      if (d / 10  !=  denom10)
		return 0; /* overflow */
	      denom10 = d;
	    }
	  s = parse_fixed (s1, (int) (s - s1), &num10);
	  product = num10 * resolution;
	  f = (product + (denom10 >> 1)) / denom10;
	  f -= f & (product % denom10  ==  denom10 >> 1); /* round to even */
	  if (f < 0  ||  product/resolution != num10)
	    return 0; /* overflow */
	}
      *fres = f;
      return s;
    }
  return 0;
}

/* Parse an initial prefix of S; it must denote a time zone.
   Set *ZONE to the number of seconds east of GMT,
   or to TM_LOCAL_ZONE if it is the local time zone.
   Return the first character after the prefix, or 0 if it wasn't parsed.  */
char *
parzone (s, zone)
     char const *s;
     long *zone;
{
  char const *s1;
  char sign;
  int hh, mm, ss;
  int minutes_east_of_UTC;
  int trailing_DST;
  long offset, z;

  /* The formats are LT, n, n DST, nDST, no, o
     where n is a time zone name
     and o is a time zone offset of the form [-+]hh[:mm[:ss]].  */
  switch (*s)
    {
    case '-':
    case '+':
      z = 0;
      break;

    default:
      minutes_east_of_UTC = lookup (s, zone_names);
      if (minutes_east_of_UTC == -1)
	return 0;

      /* Don't bother to check rest of spelling,
	 but look for an embedded "DST".  */
      trailing_DST = 0;
      while (ISALPHA ((unsigned char) *s))
	{
	  if ((*s == 'D' || *s == 'd') && lookup (s, dst_names))
	    trailing_DST = 1;
	  s++;
	  s += *s == '.';
	}

      /* Don't modify LT.  */
      if (minutes_east_of_UTC == 1)
	{
	  *zone = TM_LOCAL_ZONE;
	  return (char *) s;
	}

      z = minutes_east_of_UTC * 60L;
      s1 = s;

      /* Look for trailing "DST" or " DST".  */
      while (ISSPACE ((unsigned char) *s))
	s++;
      if (lookup (s, dst_names))
	{
	  while (ISALPHA ((unsigned char) *s))
	    {
	      s++;
	      s += *s == '.';
	    }
	  trailing_DST = 1;
	}

      if (trailing_DST)
	{
	  *zone = z + 60*60;
	  return (char *) s;
	}

      s = s1;

      switch (*s)
	{
	case '-':
	case '+':
	  break;

	default:
	  *zone = z;
	  return (char *) s;
	}

      break;
    }

  sign = *s++;

  if (! (s = parse_ranged (s, 2, 0, 23, &hh)))
    return 0;
  mm = ss = 0;
  if (*s == ':')
    s++;
  if (ISDIGIT (*s))
    {
      if (! (s = parse_ranged (s, 2, 0, 59, &mm)))
	return 0;
      if (*s == ':' && s[-3] == ':' && ISDIGIT (s[1])
	  && ! (s = parse_ranged (s + 1, 2, 0, 59, &ss)))
	return 0;
    }
  if (ISDIGIT (*s))
    return 0;
  offset = (hh * 60 + mm) * 60L + ss;
  *zone = z + (sign == '-' ? -offset : offset);
  /* ?? Are fractions allowed here?  If so, they're not implemented.  */
  return (char *) s;
}

/* Parse an initial prefix of S, matching the pattern whose code is C.
   Set *T accordingly.
   Return the first character after the prefix, or 0 if it wasn't parsed.  */
static char const *
parse_pattern_letter (s, c, t)
     char const *s;
     int c;
     struct partime *t;
{
  char const *s0 = s;

  switch (c)
    {
    case '$': /* The next character must be a non-digit.  */
      if (ISDIGIT (*s))
	return 0;
      break;

    case '-':
    case '/':
    case ':':
      /* These characters stand for themselves.  */
      if (*s++ != c)
	return 0;
      break;

    case '4': /* 4-digit year */
      s = parse_fixed (s, 4, &t->tm.tm_year);
      break;

    case ';': /* The next character must be a non-digit, and cannot be ':'.  */
      if (ISDIGIT (*s) || *s == ':')
	return 0;
      break;

    case '=': /* optional '-' */
      s += *s == '-';
      break;

    case 'A': /* AM or PM */
      /* This matches the regular expression [AaPp]\.?([Mm]\.?)?.
         It must not be followed by a letter or digit;
         otherwise it would match prefixes of strings like "PST".  */
      switch (*s)
	{
	case 'A':
	case 'a':
	  if (t->tm.tm_hour == 12)
	    t->tm.tm_hour = 0;
	  break;

	case 'P':
	case 'p':
	  if (t->tm.tm_hour != 12)
	    t->tm.tm_hour += 12;
	  break;

	default:
	  return 0;
	}
      s++;
      s += *s == '.';
      switch (*s)
	{
	case 'M':
	case 'm':
	  s++;
	  s += *s == '.';
	  break;
	}
      if (ISALNUM ((unsigned char) *s))
	return 0;
      break;

    case 'D': /* day of month [01-31] */
      s = parse_ranged (s, 2, 1, 31, &t->tm.tm_mday);
      break;

    case 'd': /* day of year [001-366] */
      s = parse_ranged (s, 3, 1, 366, &t->tm.tm_yday);
      t->tm.tm_yday--;
      break;

    case 'E': /* traditional day of month [1-9, 01-31] */
      s = parse_ranged (s, (ISDIGIT (s[0]) && ISDIGIT (s[1])) + 1, 1, 31,
			&t->tm.tm_mday);
      break;

    case 'h': /* hour [00-23] */
      s = parse_ranged (s, 2, 0, 23, &t->tm.tm_hour);
      break;

    case 'H': /* hour [00-23 followed by optional fraction] */
      {
	int frac;
	s = parse_decimal (s, 2, 0, 23, 60 * 60, &t->tm.tm_hour, &frac);
	t->tm.tm_min = frac / 60;
	t->tm.tm_sec = frac % 60;
      }
      break;

    case 'i': /* ordinal day number, e.g. "3rd" */
      s = parse_varying (s, &t->wday_ordinal);
      if (s == s0)
	return 0;
      while (ISALPHA ((unsigned char) *s))
	s++;
      break;

    case 'L': /* minute [00-59 followed by optional fraction] */
      s = parse_decimal (s, 2, 0, 59, 60, &t->tm.tm_min, &t->tm.tm_sec);
      break;

    case 'm': /* minute [00-59] */
      s = parse_ranged (s, 2, 0, 59, &t->tm.tm_min);
      break;

    case 'M': /* month [01-12] */
      s = parse_ranged (s, 2, 1, 12, &t->tm.tm_mon);
      t->tm.tm_mon--;
      break;

    case 'n': /* traditional month [1-9, 01-12] */
      s = parse_ranged (s, (ISDIGIT (s[0]) && ISDIGIT (s[1])) + 1, 1, 12,
			&t->tm.tm_mon);
      t->tm.tm_mon--;
      break;

    case 'N': /* month name [e.g. "Jan"] */
      if (! TM_DEFINED (t->tm.tm_mon = lookup (s, month_names)))
	return 0;
      /* Don't bother to check rest of spelling.  */
      while (ISALPHA ((unsigned char) *s))
	s++;
      break;

    case 'r': /* year % 10 (remainder in origin-0 decade) [0-9] */
      s = parse_fixed (s, 1, &t->tm.tm_year);
      t->ymodulus = 10;
      break;

    case_R:
    case 'R': /* year % 100 (remainder in origin-0 century) [00-99] */
      s = parse_fixed (s, 2, &t->tm.tm_year);
      t->ymodulus = 100;
      break;

    case 's': /* second [00-60 followed by optional fraction] */
      {
	int frac;
	s = parse_decimal (s, 2, 0, 60, 1, &t->tm.tm_sec, &frac);
	t->tm.tm_sec += frac;
      }
      break;

    case 'T': /* 'T' or 't' */
      switch (*s++)
	{
	case 'T':
	case 't':
	  break;
	default:
	  return 0;
	}
      break;

    case 't': /* traditional hour [1-9 or 01-12] */
      s = parse_ranged (s, (ISDIGIT (s[0]) && ISDIGIT (s[1])) + 1, 1, 12,
			&t->tm.tm_hour);
      break;

    case 'u': /* relative unit */
      {
	int i;
	int n;
	int negative = 0;
	switch (*s)
	  {
	    case '-': negative = 1;
	    /* Fall through.  */
	    case '+': s++;
	  }
	if (ISDIGIT (*s))
	  s = parse_varying (s, &n);
	else if (s == s0)
	  n = 1;
	else
	  return 0;
	if (negative)
	  n = -n;
	while (!ISALNUM ((unsigned char) *s))
	  s++;
	i = lookup (s, relative_units);
	if (!TM_DEFINED (i))
	  return 0;
	* (int *) ((char *) &t->tmr + RELATIVE_OFFSET (i))
	  += n * RELATIVE_MULTIPLIER (i);
	while (ISALPHA ((unsigned char) *s))
	  s++;
	while (! ISALNUM ((unsigned char) *s) && *s)
	  s++;
	if (TM_DEFINED (lookup (s, ago)))
	  {
	    t->tmr.tm_sec  = - t->tmr.tm_sec;
	    t->tmr.tm_min  = - t->tmr.tm_min;
	    t->tmr.tm_hour = - t->tmr.tm_hour;
	    t->tmr.tm_mday = - t->tmr.tm_mday;
	    t->tmr.tm_mon  = - t->tmr.tm_mon;
	    t->tmr.tm_year = - t->tmr.tm_year;
	    while (ISALPHA ((unsigned char) *s))
	      s++;
	  }
	break;
      }

    case 'w': /* 'W' or 'w' only (stands for current week) */
      switch (*s++)
	{
	case 'W':
	case 'w':
	  break;
	default:
	  return 0;
	}
      break;

    case 'W': /* 'W' or 'w', followed by a week of year [00-53] */
      switch (*s++)
	{
	case 'W':
	case 'w':
	  break;
	default:
	  return 0;
	}
      s = parse_ranged (s, 2, 0, 53, &t->yweek);
      break;

    case 'X': /* weekday (1=Mon ... 7=Sun) [1-7] */
      s = parse_ranged (s, 1, 1, 7, &t->tm.tm_wday);
      t->tm.tm_wday--;
      break;

    case 'x': /* weekday name [e.g. "Sun"] */
      if (! TM_DEFINED (t->tm.tm_wday = lookup (s, weekday_names)))
	return 0;
      /* Don't bother to check rest of spelling.  */
      while (ISALPHA ((unsigned char) *s))
	s++;
      break;

    case 'y': /* either R or Y */
      if (ISDIGIT (s[0]) && ISDIGIT (s[1]) && ! ISDIGIT (s[2]))
	goto case_R;
      /* fall into */
    case 'Y': /* year in full [4 or more digits] */
      s = parse_varying (s, &t->tm.tm_year);
      if (s - s0 < 4)
	return 0;
      break;

    case 'Z': /* time zone */
      s = parzone (s, &t->zone);
      break;

    case '_': /* possibly empty sequence of non-alphanumerics */
      while (! ISALNUM ((unsigned char) *s) && *s)
	s++;
      break;

    default: /* bad pattern */
      return 0;
    }

  return s;
}

/* If there is no conflict, merge into *T the additional information in *U
   and return 0.  Otherwise do nothing and return -1.  */
static int
merge_partime (t, u)
     struct partime *t;
     struct partime const *u;
{
# define conflict(a,b) ((a) != (b)  &&  TM_DEFINED (a)  &&  TM_DEFINED (b))
  if (conflict (t->tm.tm_sec, u->tm.tm_sec)
      || conflict (t->tm.tm_min, u->tm.tm_min)
      || conflict (t->tm.tm_hour, u->tm.tm_hour)
      || conflict (t->tm.tm_mday, u->tm.tm_mday)
      || conflict (t->tm.tm_mon, u->tm.tm_mon)
      || conflict (t->tm.tm_year, u->tm.tm_year)
      || conflict (t->tm.tm_wday, u->tm.tm_wday)
      || conflict (t->tm.tm_yday, u->tm.tm_yday)
      || conflict (t->ymodulus, u->ymodulus)
      || conflict (t->yweek, u->yweek)
      || (t->zone != u->zone
	  && t->zone != TM_UNDEFINED_ZONE
	  && u->zone != TM_UNDEFINED_ZONE))
    return -1;
# undef conflict
# define merge_(a,b) if (TM_DEFINED (b)) (a) = (b);
  merge_ (t->tm.tm_sec, u->tm.tm_sec)
  merge_ (t->tm.tm_min, u->tm.tm_min)
  merge_ (t->tm.tm_hour, u->tm.tm_hour)
  merge_ (t->tm.tm_mday, u->tm.tm_mday)
  merge_ (t->tm.tm_mon, u->tm.tm_mon)
  merge_ (t->tm.tm_year, u->tm.tm_year)
  merge_ (t->tm.tm_wday, u->tm.tm_wday)
  merge_ (t->tm.tm_yday, u->tm.tm_yday)
  merge_ (t->ymodulus, u->ymodulus)
  merge_ (t->yweek, u->yweek)
# undef merge_
  t->tmr.tm_sec += u->tmr.tm_sec;
  t->tmr.tm_min += u->tmr.tm_min;
  t->tmr.tm_hour += u->tmr.tm_hour;
  t->tmr.tm_mday += u->tmr.tm_mday;
  t->tmr.tm_mon += u->tmr.tm_mon;
  t->tmr.tm_year += u->tmr.tm_year;
  if (u->zone != TM_UNDEFINED_ZONE)
    t->zone = u->zone;
  return 0;
}

/* Parse a date/time prefix of S, putting the parsed result into *T.
   Return the first character after the prefix.
   The prefix may contain no useful information;
   in that case, *T will contain only undefined values.  */
char *
partime (s, t)
     char const *s;
     struct partime *t;
{
  struct partime p;

  undefine (t);

  while (*s)
    {
      char const *patterns = time_patterns;
      char const *s1;

      do
	{
	  if (! (s1 = parse_prefix (s, &patterns, &p)))
	    return (char *) s;
	}
      while (merge_partime (t, &p) != 0);

      s = s1;
    }

  return (char *) s;
}
