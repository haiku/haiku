/* This file is NOT part of Wget, but is used by Wget on the systems
   where vsnprintf() is not defined.  It has been written by Patrick
   Powell and modified by other people.  All the copyright and other
   notices have been left intact.

   My changes are documented at the bottom, along with other changes.
   I hereby place my modifications to this file under the public
   domain.  */

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Andrew Tridgell (tridge@samba.org) Oct 1998
 *    fixed handling of %.0f
 *    added test for HAVE_LONG_DOUBLE
 *
 *  Russ Allbery <rra@stanford.edu> 2000-08-26
 *    fixed return value to comply with C99
 *    fixed handling of snprintf(NULL, ...)
 *
 *  Hrvoje Niksic <hniksic@xemacs.org> 2000-11-04
 *    include <config.h> instead of "config.h".
 *    moved TEST_SNPRINTF stuff out of HAVE_SNPRINTF ifdef.
 *    include <stdio.h> for NULL.
 *    added support and test cases for long long.
 *    don't declare argument types to (v)snprintf if stdarg is not used.
 *    use int instead of short int as 2nd arg to va_arg.
 *
 *  alexk (INN) 2002-08-21
 *    use LLONG in fmtfp to handle more characters during floating
 *    point conversion.
 *
 *  herb (Samba) 2002-12-19
 *    actually print args for %g and %e
 *
 *  Hrvoje Niksic <hniksic@xemacs.org> 2005-04-15
 *    use the PARAMS macro to handle prototypes.
 *    write function definitions in the ansi2knr-friendly way.
 *    if string precision is specified, don't read VALUE past it.
 *    fix bug in fmtfp that caused 0.01 to be printed as 0.1.
 *    don't include <ctype.h> because none of it is used.
 *    interpret precision as number of significant digits with %g
 *    omit trailing decimal zeros with %g
 *
 **************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* For testing purposes, always compile in the code. */
#ifdef TEST_SNPRINTF
# undef HAVE_SNPRINTF
# undef HAVE_VSNPRINTF
# ifndef SIZEOF_LONG_LONG
#  ifdef __GNUC__
#   define SIZEOF_LONG_LONG 8
#  endif
# endif
#endif

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#include <sys/types.h>
#include <stdio.h>		/* for NULL */

/* varargs declarations: */

#if defined(HAVE_STDARG_H)
# include <stdarg.h>
# define HAVE_STDARGS    /* let's hope that works everywhere (mj) */
# define VA_LOCAL_DECL   va_list ap
# define VA_START(f)     va_start(ap, f)
# define VA_SHIFT(v,t)  ;   /* no-op for ANSI */
# define VA_END          va_end(ap)
#else
# include <varargs.h>
# undef HAVE_STDARGS
# define VA_LOCAL_DECL   va_list ap
# define VA_START(f)     va_start(ap)      /* f is ignored! */
# define VA_SHIFT(v,t) v = va_arg(ap,t)
# define VA_END        va_end(ap)
#endif

#ifdef HAVE_LONG_DOUBLE
#define LDOUBLE long double
#else
#define LDOUBLE double
#endif

#if SIZEOF_LONG_LONG != 0
# define LLONG long long
#else
# define LLONG long
#endif

/* If we're running the test suite, rename snprintf and vsnprintf to
   avoid conflicts with the system version.  */
#ifdef TEST_SNPRINTF
# define snprintf test_snprintf
# define vsnprintf test_vsnprintf
#endif

#ifdef HAVE_STDARGS
int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);
#else
int snprintf ();
int vsnprintf ();
#endif

#ifndef PARAMS
# define PARAMS(x) x
#endif

static int dopr PARAMS ((char *buffer, size_t maxlen, const char *format, 
			 va_list args));
static int fmtstr PARAMS ((char *buffer, size_t *currlen, size_t maxlen,
			   const char *value, int flags, int min, int max));
static int fmtint PARAMS ((char *buffer, size_t *currlen, size_t maxlen,
			   LLONG value, int base, int min, int max, int flags));
static int fmtfp PARAMS ((char *buffer, size_t *currlen, size_t maxlen,
			  LDOUBLE fvalue, int min, int max, int flags));
static int dopr_outch PARAMS ((char *buffer, size_t *currlen, size_t maxlen,
			       char c));

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_MOD_L   6
#define DP_S_CONV    7
#define DP_S_DONE    8

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)
#define DP_F_FP_G 	(1 << 7)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LLONG   3
#define DP_C_LDOUBLE 4

#define char_to_int(p) (p - '0')
#define MAX(p,q) ((p >= q) ? p : q)
#define MIN(p,q) ((p <= q) ? p : q)

static int
dopr (char *buffer, size_t maxlen, const char *format, va_list args)
{
  char ch;
  LLONG value;
  LDOUBLE fvalue;
  char *strvalue;
  int min;
  int max;
  int state;
  int flags;
  int cflags;
  int total;
  size_t currlen;
  
  state = DP_S_DEFAULT;
  currlen = flags = cflags = min = 0;
  max = -1;
  ch = *format++;
  total = 0;

  while (state != DP_S_DONE)
  {
    if (ch == '\0')
      state = DP_S_DONE;

    switch(state) 
    {
    case DP_S_DEFAULT:
      if (ch == '%') 
	state = DP_S_FLAGS;
      else 
	total += dopr_outch (buffer, &currlen, maxlen, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) 
      {
      case '-':
	flags |= DP_F_MINUS;
        ch = *format++;
	break;
      case '+':
	flags |= DP_F_PLUS;
        ch = *format++;
	break;
      case ' ':
	flags |= DP_F_SPACE;
        ch = *format++;
	break;
      case '#':
	flags |= DP_F_NUM;
        ch = *format++;
	break;
      case '0':
	flags |= DP_F_ZERO;
        ch = *format++;
	break;
      default:
	state = DP_S_MIN;
	break;
      }
      break;
    case DP_S_MIN:
      if ('0' <= ch && ch <= '9')
      {
	min = 10*min + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	min = va_arg (args, int);
	ch = *format++;
	state = DP_S_DOT;
      } 
      else 
	state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if (ch == '.') 
      {
	state = DP_S_MAX;
	ch = *format++;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if ('0' <= ch && ch <= '9')
      {
	if (max < 0)
	  max = 0;
	max = 10*max + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	max = va_arg (args, int);
	ch = *format++;
	state = DP_S_MOD;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MOD:
      switch (ch) 
      {
      case 'h':
	cflags = DP_C_SHORT;
	ch = *format++;
	break;
      case 'l':
	cflags = DP_C_LONG;
	ch = *format++;
	break;
      case 'L':
	cflags = DP_C_LDOUBLE;
	ch = *format++;
	break;
      default:
	break;
      }
      if (cflags != DP_C_LONG)
	state = DP_S_CONV;
      else
	state = DP_S_MOD_L;
      break;
    case DP_S_MOD_L:
      switch (ch)
	{
	case 'l':
	  cflags = DP_C_LLONG;
	  ch = *format++;
	  break;
	default:
	  break;
	}
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) 
      {
      case 'd':
      case 'i':
	if (cflags == DP_C_SHORT) 
	  value = (short int) va_arg (args, int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, LLONG);
	else
	  value = va_arg (args, int);
	total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'o':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = (unsigned short int) va_arg (args, unsigned int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, unsigned LLONG);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
	break;
      case 'u':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = (unsigned short int) va_arg (args, unsigned int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, unsigned LLONG);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'X':
	flags |= DP_F_UP;
      case 'x':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = (unsigned short int) va_arg (args, unsigned int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, unsigned LLONG);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
	break;
      case 'f':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	total += fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'E':
	flags |= DP_F_UP;
      case 'e':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	total += fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'G':
	flags |= DP_F_UP;
      case 'g':
	flags |= DP_F_FP_G;
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	if (max == 0)
	  /* C99 says: if precision [for %g] is zero, it is taken as one */
	  max = 1;
	total += fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'c':
	total += dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
	break;
      case 's':
	strvalue = va_arg (args, char *);
	total += fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
	break;
      case 'p':
	strvalue = va_arg (args, void *);
	total += fmtint (buffer, &currlen, maxlen, (long) strvalue, 16, min,
                         max, flags);
	break;
      case 'n':
	if (cflags == DP_C_SHORT) 
	{
	  short int *num;
	  num = va_arg (args, short int *);
	  *num = currlen;
        }
	else if (cflags == DP_C_LONG) 
	{
	  long int *num;
	  num = va_arg (args, long int *);
	  *num = currlen;
        } 
	else if (cflags == DP_C_LLONG) 
	{
	  LLONG *num;
	  num = va_arg (args, LLONG *);
	  *num = currlen;
        } 
	else 
	{
	  int *num;
	  num = va_arg (args, int *);
	  *num = currlen;
        }
	break;
      case '%':
	total += dopr_outch (buffer, &currlen, maxlen, ch);
	break;
      case 'w':
	/* not supported yet, treat as next char */
	ch = *format++;
	break;
      default:
	/* Unknown, skip */
	break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      flags = cflags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default:
      /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if (buffer != NULL)
  {
    if (currlen < maxlen - 1) 
      buffer[currlen] = '\0';
    else 
      buffer[maxlen - 1] = '\0';
  }
  return total;
}

static int
fmtstr (char *buffer, size_t *currlen, size_t maxlen,
	const char *value, int flags, int min, int max)
{
  int padlen, strln;     /* amount to pad */
  int cnt = 0;
  int total = 0;
  
  if (value == 0)
  {
    value = "(null)";
  }

  if (max < 0)
    strln = strlen (value);
  else
    /* When precision is specified, don't read VALUE past precision. */
    /*strln = strnlen (value, max);*/
    for (strln = 0; strln < max && value[strln]; ++strln);
  padlen = min - strln;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justify */

  while (padlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  while (*value && ((max < 0) || (cnt < max)))
  {
    total += dopr_outch (buffer, currlen, maxlen, *value++);
    ++cnt;
  }
  while (padlen < 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }
  return total;
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static int
fmtint (char *buffer, size_t *currlen, size_t maxlen,
	LLONG value, int base, int min, int max, int flags)
{
  int signvalue = 0;
  unsigned LLONG uvalue;
  char convert[24];
  int place = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */
  const char *digits;
  int total = 0;
  
  if (max < 0)
    max = 0;

  uvalue = value;

  if(!(flags & DP_F_UNSIGNED))
  {
    if( value < 0 ) {
      signvalue = '-';
      uvalue = -value;
    }
    else
      if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
	signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';
  }
  
  if (flags & DP_F_UP)
    /* Should characters be upper case? */
    digits = "0123456789ABCDEF";
  else
    digits = "0123456789abcdef";

  do {
    convert[place++] = digits[uvalue % (unsigned)base];
    uvalue = (uvalue / (unsigned)base );
  } while(uvalue && (place < sizeof (convert)));
  if (place == sizeof (convert)) place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
  if (zpadlen < 0) zpadlen = 0;
  if (spadlen < 0) spadlen = 0;
  if (flags & DP_F_ZERO)
  {
    zpadlen = MAX(zpadlen, spadlen);
    spadlen = 0;
  }
  if (flags & DP_F_MINUS) 
    spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
      zpadlen, spadlen, min, max, place));
#endif

  /* Spaces */
  while (spadlen > 0) 
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --spadlen;
  }

  /* Sign */
  if (signvalue) 
    total += dopr_outch (buffer, currlen, maxlen, signvalue);

  /* Zeros */
  if (zpadlen > 0) 
  {
    while (zpadlen > 0)
    {
      total += dopr_outch (buffer, currlen, maxlen, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0) 
    total += dopr_outch (buffer, currlen, maxlen, convert[--place]);
  
  /* Left Justified spaces */
  while (spadlen < 0) {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++spadlen;
  }

  return total;
}

static LDOUBLE
abs_val (LDOUBLE value)
{
  LDOUBLE result = value;

  if (value < 0)
    result = -value;

  return result;
}

static LDOUBLE
pow10 (int exp)
{
  LDOUBLE result = 1;

  while (exp)
  {
    result *= 10;
    exp--;
  }
  
  return result;
}

static LLONG
round (LDOUBLE value)
{
  LLONG intpart;

  intpart = value;
  value = value - intpart;
  if (value >= 0.5)
    intpart++;

  return intpart;
}

static int
fmtfp (char *buffer, size_t *currlen, size_t maxlen,
       LDOUBLE fvalue, int min, int max, int flags)
{
  int signvalue = 0;
  LDOUBLE ufvalue;
  char iconvert[24];
  char fconvert[24];
  int iplace = 0;
  int fplace = 0;
  int padlen = 0; /* amount to pad */
  int zpadlen = 0; 
  int total = 0;
  LLONG intpart;
  LLONG fracpart;
  LLONG mask10;
  int leadingfrac0s = 0; /* zeros at the start of fractional part */
  int omitzeros = 0;
  int omitcount = 0;
  
  /* 
   * AIX manpage says the default is 0, but Solaris says the default
   * is 6, and sprintf on AIX defaults to 6
   */
  if (max < 0)
    max = 6;

  ufvalue = abs_val (fvalue);

  if (fvalue < 0)
    signvalue = '-';
  else
    if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
      signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';

#if 0
  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */
#endif

  intpart = ufvalue;

  /* With %g precision is the number of significant digits, which
     includes the digits in intpart. */
  if (flags & DP_F_FP_G)
    {
      if (intpart != 0)
	{
	  /* For each digit of INTPART, print one less fractional digit. */
	  LLONG temp = intpart;
	  for (temp = intpart; temp != 0; temp /= 10)
	    --max;
	  if (max < 0)
	    max = 0;
	}
      else
	{
	  /* For each leading 0 in fractional part, print one more
	     fractional digit. */
	  LDOUBLE temp;
	  if (ufvalue != 0)
	    for (temp = ufvalue; temp < 0.1; temp *= 10)
	      ++max;
	}
    }

  /* C99: trailing zeros are removed from the fractional portion of the
     result unless the # flag is specified */
  if ((flags & DP_F_FP_G) && !(flags & DP_F_NUM))
    omitzeros = 1;

#if SIZEOF_LONG_LONG > 0
# define MAX_DIGITS 18		/* grok more digits with long long */
#else
# define MAX_DIGITS 9		/* just long */
#endif

  /* 
   * Sorry, we only support several digits past the decimal because of
   * our conversion method
   */
  if (max > MAX_DIGITS)
    max = MAX_DIGITS;

  /* Factor of 10 with the needed number of digits, e.g. 1000 for max==3 */
  mask10 = pow10 (max);

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */
  fracpart = round (mask10 * (ufvalue - intpart));

  if (fracpart >= mask10)
  {
    intpart++;
    fracpart -= mask10;
  }
  else if (fracpart != 0)
    /* If fracpart has less digits than the 10* mask, we need to
       manually insert leading 0s.  For example 2.01's fractional part
       requires one leading zero to distinguish it from 2.1. */
    while (fracpart < mask10 / 10)
      {
	++leadingfrac0s;
	mask10 /= 10;
      }

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "fmtfp: %f =? %d.%d\n", fvalue, intpart, fracpart));
#endif

  /* Convert integer part */
  do {
    iconvert[iplace++] = '0' + intpart % 10;
    intpart = (intpart / 10);
  } while(intpart && (iplace < sizeof(iconvert)));
  if (iplace == sizeof(iconvert)) iplace--;
  iconvert[iplace] = 0;

  /* Convert fractional part */
  do {
    fconvert[fplace++] = '0' + fracpart % 10;
    fracpart = (fracpart / 10);
  } while(fracpart && (fplace < sizeof(fconvert)));
  while (leadingfrac0s-- > 0 && fplace < sizeof(fconvert))
    fconvert[fplace++] = '0';
  if (fplace == sizeof(fconvert)) fplace--;
  fconvert[fplace] = 0;
  if (omitzeros)
    while (omitcount < fplace && fconvert[omitcount] == '0')
      ++omitcount;

  /* -1 for decimal point, another -1 if we are printing a sign */
  padlen = min - iplace - (max - omitcount) - 1 - ((signvalue) ? 1 : 0);
  if (!omitzeros)
    zpadlen = max - fplace;
  if (zpadlen < 0)
    zpadlen = 0;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justifty */

  if ((flags & DP_F_ZERO) && (padlen > 0)) 
  {
    if (signvalue) 
    {
      total += dopr_outch (buffer, currlen, maxlen, signvalue);
      --padlen;
      signvalue = 0;
    }
    while (padlen > 0)
    {
      total += dopr_outch (buffer, currlen, maxlen, '0');
      --padlen;
    }
  }
  while (padlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  if (signvalue) 
    total += dopr_outch (buffer, currlen, maxlen, signvalue);

  while (iplace > 0) 
    total += dopr_outch (buffer, currlen, maxlen, iconvert[--iplace]);

  /*
   * Decimal point.  This should probably use locale to find the correct
   * char to print out.
   */
  if (max > 0 && (fplace > omitcount || zpadlen > 0))
  {
    total += dopr_outch (buffer, currlen, maxlen, '.');

    while (fplace > omitcount) 
      total += dopr_outch (buffer, currlen, maxlen, fconvert[--fplace]);
  }

  while (zpadlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, '0');
    --zpadlen;
  }

  while (padlen < 0) 
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }

  return total;
}

static int
dopr_outch (char *buffer, size_t *currlen, size_t maxlen, char c)
{
  if (*currlen + 1 < maxlen)
    buffer[(*currlen)++] = c;
  return 1;
}

#ifndef HAVE_VSNPRINTF
int
vsnprintf (char *str, size_t count, const char *fmt, va_list args)
{
  if (str != NULL)
    str[0] = 0;
  return dopr(str, count, fmt, args);
}
#endif /* !HAVE_VSNPRINTF */

#ifndef HAVE_SNPRINTF
int
snprintf (char *str, size_t count,const char *fmt,...)
{
#ifndef HAVE_STDARGS
  char *str;
  size_t count;
  char *fmt;
#endif
  VA_LOCAL_DECL;
  int total;
    
  VA_START (fmt);
  VA_SHIFT (str, char *);
  VA_SHIFT (count, size_t );
  VA_SHIFT (fmt, char *);
  total = vsnprintf(str, count, fmt, ap);
  VA_END;
  return total;
}
#endif /* !HAVE_SNPRINTF */
#endif /* !HAVE_SNPRINTF || !HAVE_VSNPRINTF */

#ifdef TEST_SNPRINTF

# ifndef LONG_STRING
#  define LONG_STRING 1024
# endif

int
main (void)
{
  char buf1[LONG_STRING];
  char buf2[LONG_STRING];
  char *fp_fmt[] = {
    /* %f formats */
    "%f",
    "%-1.5f",
    "%1.5f",
    "%123.9f",
    "%10.5f",
    "% 10.5f",
    "%+22.9f",
    "%+4.9f",
    "%01.3f",
    "%4f",
    "%3.1f",
    "%3.2f",
    "%.0f",
    "%.1f",
    "%#10.1f",
#if SIZEOF_LONG_LONG != 0
    "%.16f",
    "%18.16f",
    "%-16.16f",
#endif
    /* %g formats */
    "%g",
    "%1.5g",
    "%-1.5g",
    "%.9g",
    "%123.9g",
    "%#123.9g",
#if SIZEOF_LONG_LONG != 0
    "%.16g",
    "%20.16g",
#endif
    NULL
  };
  double fp_nums[] = { -1.5, 134.21, 91340.2, 341.1234, 0203.9, 0.96, 0.996, 
		       0.9996, 1.996, 4.136, 0.00205, 0.0001, 321.000009,
		       0};
  char *int_fmt[] = {
    "%-1.5d",
    "%1.5d",
    "%123.9d",
    "%5.5d",
    "%10.5d",
    "% 10.5d",
    "%+22.33d",
    "%01.3d",
    "%4d",
    NULL
  };
  long int_nums[] = { -1, 134, 91340, 341, 0203, 0};
#if SIZEOF_LONG_LONG != 0
  char *llong_fmt[] = {
    "%lld",		"%llu",
    "%-1.5lld",		"%-1.5llu",
    "%1.5lld",		"%1.5llu",
    "%123.9lld",	"%123.9llu",
    "%5.5lld",		"%5.5llu",
    "%10.5lld",		"%10.5llu",
    "% 10.5lld",	"% 10.5llu",
    "%+22.33lld",	"%+22.33llu",
    "%01.3lld",		"%01.3llu",
    "%4lld",		"%4llu",
    NULL
  };
  long long llong_nums[] = {
    ~(long long)0,		/* all-1 bit pattern */
    (~(unsigned long long)0) >> 1, /* largest signed long long */
    /* random... */
    -150, 134, 91340, 341,
    0
  };
#endif
  int x, y;
  int fail = 0;
  int num = 0;

  printf ("Testing snprintf format codes against system sprintf...\n");

  for (x = 0; fp_fmt[x] != NULL ; x++)
    for (y = 0; fp_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), fp_fmt[x], fp_nums[y]);
      sprintf (buf2, fp_fmt[x], fp_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    fp_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }

  for (x = 0; int_fmt[x] != NULL ; x++)
    for (y = 0; int_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), int_fmt[x], int_nums[y]);
      sprintf (buf2, int_fmt[x], int_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    int_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }

#if SIZEOF_LONG_LONG != 0
  for (x = 0; llong_fmt[x] != NULL ; x++)
    for (y = 0; llong_nums[y] != 0 ; y++)
    {
      snprintf (buf1, sizeof (buf1), llong_fmt[x], llong_nums[y]);
      sprintf (buf2, llong_fmt[x], llong_nums[y]);
      if (strcmp (buf1, buf2))
      {
	printf("snprintf doesn't match Format: %s\n\tsnprintf = %s\n\tsprintf  = %s\n", 
	    llong_fmt[x], buf1, buf2);
	fail++;
      }
      num++;
    }
#endif

  printf ("%d tests failed out of %d.\n", fail, num);
  return 0;
}
#endif /* TEST_SNPRINTF */
