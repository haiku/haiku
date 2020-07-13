/* Read decimal floating point numbers.
   This file is part of the GNU C Library.
   Copyright (C) 1995,96,97,98,99,2000,2001 Free Software Foundation, Inc.
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

/* Configuration part.  These macros are defined by `strtold.c',
   `strtof.c', `wcstod.c', `wcstold.c', and `wcstof.c' to produce the
   `long double' and `float' versions of the reader.  */
#ifndef FLOAT
# define FLOAT		double
# define FLT		DBL
# ifdef USE_WIDE_CHAR
#  ifdef USE_IN_EXTENDED_LOCALE_MODEL
#   define STRTOF	__wcstod_l
#  else
#   define STRTOF	wcstod
#  endif
# else
#  ifdef USE_IN_EXTENDED_LOCALE_MODEL
#   define STRTOF	__strtod_l
#  else
#   define STRTOF	strtod
#  endif
# endif
# define MPN2FLOAT	__mpn_construct_double
# define FLOAT_HUGE_VAL	HUGE_VAL
# define SET_MANTISSA(flt, mant) \
  do { union ieee754_double u;						      \
       u.d = (flt);							      \
       if ((mant & 0xfffffffffffffULL) == 0)				      \
	 mant = 0x8000000000000ULL;					      \
       u.ieee.mantissa0 = ((mant) >> 32) & 0xfffff;			      \
       u.ieee.mantissa1 = (mant) & 0xffffffff;				      \
       (flt) = u.d;							      \
  } while (0)
#endif
/* End of configuration part.  */

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <ieee754.h>
#include "../locale/localeinfo.h"
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The gmp headers need some configuration frobs.  */
#define HAVE_ALLOCA 1

#include <gmp.h>
#include <gmp-impl.h>
#include <gmp-mparam.h>
#include <longlong.h>
#include "fpioconst.h"

#define NDEBUG 1
#include <assert.h>


/* We use this code also for the extended locale handling where the
   function gets as an additional argument the locale which has to be
   used.  To access the values we have to redefine the _NL_CURRENT
   macro.  */
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# undef _NL_CURRENT
# define _NL_CURRENT(category, item) \
  (current->values[_NL_ITEM_INDEX (item)].string)
# define LOCALE_PARAM , loc
# define LOCALE_PARAM_DECL __locale_t loc;
#else
# define LOCALE_PARAM
# define LOCALE_PARAM_DECL
#endif

#if defined _LIBC || defined HAVE_WCHAR_H
# include <wchar.h>
#endif

#ifdef USE_WIDE_CHAR
# include <wctype.h>
# define STRING_TYPE wchar_t
# define CHAR_TYPE wint_t
# define L_(Ch) L##Ch
# ifdef USE_IN_EXTENDED_LOCALE_MODEL
#  define ISSPACE(Ch) __iswspace_l ((Ch), loc)
#  define ISDIGIT(Ch) __iswdigit_l ((Ch), loc)
#  define ISXDIGIT(Ch) __iswxdigit_l ((Ch), loc)
#  define TOLOWER(Ch) __towlower_l ((Ch), loc)
#  define STRNCASECMP(S1, S2, N) __wcsncasecmp_l ((S1), (S2), (N), loc)
#  define STRTOULL(S, E, B) ____wcstoull_l_internal ((S), (E), (B), 0, loc)
# else
#  define ISSPACE(Ch) iswspace (Ch)
#  define ISDIGIT(Ch) iswdigit (Ch)
#  define ISXDIGIT(Ch) iswxdigit (Ch)
#  define TOLOWER(Ch) towlower (Ch)
#  define STRNCASECMP(S1, S2, N) __wcsncasecmp ((S1), (S2), (N))
#  define STRTOULL(S, E, B) __wcstoull_internal ((S), (E), (B), 0)
# endif
#else
# define STRING_TYPE char
# define CHAR_TYPE char
# define L_(Ch) Ch
# ifdef USE_IN_EXTENDED_LOCALE_MODEL
#  define ISSPACE(Ch) __isspace_l ((Ch), loc)
#  define ISDIGIT(Ch) __isdigit_l ((Ch), loc)
#  define ISXDIGIT(Ch) __isxdigit_l ((Ch), loc)
#  define TOLOWER(Ch) __tolower_l ((Ch), loc)
#  define STRNCASECMP(S1, S2, N) __strncasecmp_l ((S1), (S2), (N), loc)
#  define STRTOULL(S, E, B) ____strtoull_l_internal ((S), (E), (B), 0, loc)
# else
#  define ISSPACE(Ch) isspace (Ch)
#  define ISDIGIT(Ch) isdigit (Ch)
#  define ISXDIGIT(Ch) isxdigit (Ch)
#  define TOLOWER(Ch) tolower (Ch)
#  define STRNCASECMP(S1, S2, N) strncasecmp ((S1), (S2), (N))
#  define STRTOULL(S, E, B) __strtoull_internal ((S), (E), 0, (B))
# endif
#endif


/* Constants we need from float.h; select the set for the FLOAT precision.  */
#define MANT_DIG	PASTE(FLT,_MANT_DIG)
#define	DIG		PASTE(FLT,_DIG)
#define	MAX_EXP		PASTE(FLT,_MAX_EXP)
#define	MIN_EXP		PASTE(FLT,_MIN_EXP)
#define MAX_10_EXP	PASTE(FLT,_MAX_10_EXP)
#define MIN_10_EXP	PASTE(FLT,_MIN_10_EXP)

/* Extra macros required to get FLT expanded before the pasting.  */
#define PASTE(a,b)	PASTE1(a,b)
#define PASTE1(a,b)	a##b

/* Function to construct a floating point number from an MP integer
   containing the fraction bits, a base 2 exponent, and a sign flag.  */
extern FLOAT MPN2FLOAT (mp_srcptr mpn, int exponent, int negative);

/* Definitions according to limb size used.  */
#if	BITS_PER_MP_LIMB == 32
#  define MAX_DIG_PER_LIMB	9
#  define MAX_FAC_PER_LIMB	1000000000UL
#elif	BITS_PER_MP_LIMB == 64
#  define MAX_DIG_PER_LIMB	19
#  define MAX_FAC_PER_LIMB	10000000000000000000UL
#else
#  error "mp_limb_t size " BITS_PER_MP_LIMB "not accounted for"
#endif


/* Local data structure.  */
static const mp_limb_t _tens_in_limb[MAX_DIG_PER_LIMB + 1] =
{    0,                   10,                   100,
     1000,                10000,                100000,
     1000000,             10000000,             100000000,
     1000000000
#if BITS_PER_MP_LIMB > 32
	       ,	   10000000000U,          100000000000U,
     1000000000000U,       10000000000000U,       100000000000000U,
     1000000000000000U,    10000000000000000U,    100000000000000000U,
     1000000000000000000U, 10000000000000000000U
#endif
#if BITS_PER_MP_LIMB > 64
  #error "Need to expand tens_in_limb table to" MAX_DIG_PER_LIMB
#endif
};

#ifndef	howmany
#define	howmany(x,y)		(((x)+((y)-1))/(y))
#endif
#define SWAP(x, y)		({ typeof(x) _tmp = x; x = y; y = _tmp; })

#define NDIG			(MAX_10_EXP - MIN_10_EXP + 2 * MANT_DIG)
#define HEXNDIG			((MAX_EXP - MIN_EXP + 7) / 8 + 2 * MANT_DIG)
#define	RETURN_LIMB_SIZE		howmany (MANT_DIG, BITS_PER_MP_LIMB)

#define RETURN(val,end)							      \
    do { if (endptr != NULL) *endptr = (STRING_TYPE *) (end);		      \
	 return val; } while (0)

/* Maximum size necessary for mpn integers to hold floating point numbers.  */
#define	MPNSIZE		(howmany (MAX_EXP + 2 * MANT_DIG, BITS_PER_MP_LIMB) \
			 + 2)
/* Declare an mpn integer variable that big.  */
#define	MPN_VAR(name)	mp_limb_t name[MPNSIZE]; mp_size_t name##size
/* Copy an mpn integer value.  */
#define MPN_ASSIGN(dst, src) \
	memcpy (dst, src, (dst##size = src##size) * sizeof (mp_limb_t))


/* Return a floating point number of the needed type according to the given
   multi-precision number after possible rounding.  */
static inline FLOAT
round_and_return (mp_limb_t *retval, int exponent, int negative,
		  mp_limb_t round_limb, mp_size_t round_bit, int more_bits)
{
  if (exponent < MIN_EXP - 1)
    {
      mp_size_t shift = MIN_EXP - 1 - exponent;

      if (shift > MANT_DIG)
	{
	  __set_errno (EDOM);
	  return 0.0;
	}

      more_bits |= (round_limb & ((((mp_limb_t) 1) << round_bit) - 1)) != 0;
      if (shift == MANT_DIG)
	/* This is a special case to handle the very seldom case where
	   the mantissa will be empty after the shift.  */
	{
	  int i;

	  round_limb = retval[RETURN_LIMB_SIZE - 1];
	  round_bit = (MANT_DIG - 1) % BITS_PER_MP_LIMB;
	  for (i = 0; i < RETURN_LIMB_SIZE; ++i)
	    more_bits |= retval[i] != 0;
	  MPN_ZERO (retval, RETURN_LIMB_SIZE);
	}
      else if (shift >= BITS_PER_MP_LIMB)
	{
	  int i;

	  round_limb = retval[(shift - 1) / BITS_PER_MP_LIMB];
	  round_bit = (shift - 1) % BITS_PER_MP_LIMB;
	  for (i = 0; i < (shift - 1) / BITS_PER_MP_LIMB; ++i)
	    more_bits |= retval[i] != 0;
	  more_bits |= ((round_limb & ((((mp_limb_t) 1) << round_bit) - 1))
			!= 0);

	  (void) __mpn_rshift (retval, &retval[shift / BITS_PER_MP_LIMB],
                               RETURN_LIMB_SIZE - (shift / BITS_PER_MP_LIMB),
                               shift % BITS_PER_MP_LIMB);
          MPN_ZERO (&retval[RETURN_LIMB_SIZE - (shift / BITS_PER_MP_LIMB)],
                    shift / BITS_PER_MP_LIMB);
	}
      else if (shift > 0)
	{
          round_limb = retval[0];
          round_bit = shift - 1;
	  (void) __mpn_rshift (retval, retval, RETURN_LIMB_SIZE, shift);
	}
      /* This is a hook for the m68k long double format, where the
	 exponent bias is the same for normalized and denormalized
	 numbers.  */
#ifndef DENORM_EXP
# define DENORM_EXP (MIN_EXP - 2)
#endif
      exponent = DENORM_EXP;
    }

  if ((round_limb & (((mp_limb_t) 1) << round_bit)) != 0
      && (more_bits || (retval[0] & 1) != 0
          || (round_limb & ((((mp_limb_t) 1) << round_bit) - 1)) != 0))
    {
      mp_limb_t cy = __mpn_add_1 (retval, retval, RETURN_LIMB_SIZE, 1);

      if (((MANT_DIG % BITS_PER_MP_LIMB) == 0 && cy) ||
          ((MANT_DIG % BITS_PER_MP_LIMB) != 0 &&
           (retval[RETURN_LIMB_SIZE - 1]
            & (((mp_limb_t) 1) << (MANT_DIG % BITS_PER_MP_LIMB))) != 0))
	{
	  ++exponent;
	  (void) __mpn_rshift (retval, retval, RETURN_LIMB_SIZE, 1);
	  retval[RETURN_LIMB_SIZE - 1]
	    |= ((mp_limb_t) 1) << ((MANT_DIG - 1) % BITS_PER_MP_LIMB);
	}
      else if (exponent == DENORM_EXP
	       && (retval[RETURN_LIMB_SIZE - 1]
		   & (((mp_limb_t) 1) << ((MANT_DIG - 1) % BITS_PER_MP_LIMB)))
	       != 0)
	  /* The number was denormalized but now normalized.  */
	exponent = MIN_EXP - 1;
    }

  if (exponent > MAX_EXP)
    return negative ? -FLOAT_HUGE_VAL : FLOAT_HUGE_VAL;

  return MPN2FLOAT (retval, exponent, negative);
}


/* Read a multi-precision integer starting at STR with exactly DIGCNT digits
   into N.  Return the size of the number limbs in NSIZE at the first
   character od the string that is not part of the integer as the function
   value.  If the EXPONENT is small enough to be taken as an additional
   factor for the resulting number (see code) multiply by it.  */
static inline const STRING_TYPE *
str_to_mpn (const STRING_TYPE *str, int digcnt, mp_limb_t *n, mp_size_t *nsize,
	    int *exponent
#ifndef USE_WIDE_CHAR
	    , const char *decimal, size_t decimal_len, const char *thousands
#endif

	    )
{
  /* Number of digits for actual limb.  */
  int cnt = 0;
  mp_limb_t low = 0;
  mp_limb_t start;

  *nsize = 0;
  assert (digcnt > 0);
  do
    {
      if (cnt == MAX_DIG_PER_LIMB)
	{
	  if (*nsize == 0)
	    {
	      n[0] = low;
	      *nsize = 1;
	    }
	  else
	    {
	      mp_limb_t cy;
	      cy = __mpn_mul_1 (n, n, *nsize, MAX_FAC_PER_LIMB);
	      cy += __mpn_add_1 (n, n, *nsize, low);
	      if (cy != 0)
		{
		  n[*nsize] = cy;
		  ++(*nsize);
		}
	    }
	  cnt = 0;
	  low = 0;
	}

      /* There might be thousands separators or radix characters in
	 the string.  But these all can be ignored because we know the
	 format of the number is correct and we have an exact number
	 of characters to read.  */
#ifdef USE_WIDE_CHAR
      if (*str < L'0' || *str > L'9')
	++str;
#else
      if (*str < '0' || *str > '9')
	{
	  int inner = 0;
	  if (thousands != NULL && *str == *thousands
	      && ({ for (inner = 1; thousands[inner] != '\0'; ++inner)
		      if (thousands[inner] != str[inner])
			break;
		    thousands[inner] == '\0'; }))
	    str += inner;
	  else
	    str += decimal_len;
	}
#endif
      low = low * 10 + *str++ - L_('0');
      ++cnt;
    }
  while (--digcnt > 0);

  if (*exponent > 0 && cnt + *exponent <= MAX_DIG_PER_LIMB)
    {
      low *= _tens_in_limb[*exponent];
      start = _tens_in_limb[cnt + *exponent];
      *exponent = 0;
    }
  else
    start = _tens_in_limb[cnt];

  if (*nsize == 0)
    {
      n[0] = low;
      *nsize = 1;
    }
  else
    {
      mp_limb_t cy;
      cy = __mpn_mul_1 (n, n, *nsize, start);
      cy += __mpn_add_1 (n, n, *nsize, low);
      if (cy != 0)
	n[(*nsize)++] = cy;
    }

  return str;
}


/* Shift {PTR, SIZE} COUNT bits to the left, and fill the vacated bits
   with the COUNT most significant bits of LIMB.

   Tege doesn't like this function so I have to write it here myself. :)
   --drepper */
static inline void
__mpn_lshift_1 (mp_limb_t *ptr, mp_size_t size, unsigned int count,
		mp_limb_t limb)
{
  if (count == BITS_PER_MP_LIMB)
    {
      /* Optimize the case of shifting by exactly a word:
	 just copy words, with no actual bit-shifting.  */
      mp_size_t i;
      for (i = size - 1; i > 0; --i)
	ptr[i] = ptr[i - 1];
      ptr[0] = limb;
    }
  else
    {
      (void) __mpn_lshift (ptr, ptr, size, count);
      ptr[0] |= limb >> (BITS_PER_MP_LIMB - count);
    }
}


#define INTERNAL(x) INTERNAL1(x)
#define INTERNAL1(x) __##x##_internal

/* This file defines a function to check for correct grouping.  */
#include "grouping.h"


/* Return a floating point number with the value of the given string NPTR.
   Set *ENDPTR to the character after the last used one.  If the number is
   smaller than the smallest representable number, set `errno' to ERANGE and
   return 0.0.  If the number is too big to be represented, set `errno' to
   ERANGE and return HUGE_VAL with the appropriate sign.  */
FLOAT
INTERNAL (STRTOF) (nptr, endptr, group LOCALE_PARAM)
     const STRING_TYPE *nptr;
     STRING_TYPE **endptr;
     int group;
     LOCALE_PARAM_DECL
{
  int negative;			/* The sign of the number.  */
  MPN_VAR (num);		/* MP representation of the number.  */
  int exponent;			/* Exponent of the number.  */

  /* Numbers starting `0X' or `0x' have to be processed with base 16.  */
  int base = 10;

  /* When we have to compute fractional digits we form a fraction with a
     second multi-precision number (and we sometimes need a second for
     temporary results).  */
  MPN_VAR (den);

  /* Representation for the return value.  */
  mp_limb_t retval[RETURN_LIMB_SIZE];
  /* Number of bits currently in result value.  */
  int bits;

  /* Running pointer after the last character processed in the string.  */
  const STRING_TYPE *cp, *tp;
  /* Start of significant part of the number.  */
  const STRING_TYPE *startp, *start_of_digits;
  /* Points at the character following the integer and fractional digits.  */
  const STRING_TYPE *expp;
  /* Total number of digit and number of digits in integer part.  */
  int dig_no, int_no, lead_zero;
  /* Contains the last character read.  */
  CHAR_TYPE c;

/* We should get wint_t from <stddef.h>, but not all GCC versions define it
   there.  So define it ourselves if it remains undefined.  */
#ifndef _WINT_T
  typedef unsigned int wint_t;
#endif
  /* The radix character of the current locale.  */
#ifdef USE_WIDE_CHAR
  wchar_t decimal;
#else
  const char *decimal;
  size_t decimal_len;
#endif
  /* The thousands character of the current locale.  */
#ifdef USE_WIDE_CHAR
  wchar_t thousands = L'\0';
#else
  const char *thousands = NULL;
#endif
  /* The numeric grouping specification of the current locale,
     in the format described in <locale.h>.  */
  const char *grouping;
  /* Used in several places.  */
  int cnt;

#ifdef USE_IN_EXTENDED_LOCALE_MODEL
  struct locale_data *current = loc->__locales[LC_NUMERIC];
#endif

  if (nptr == NULL)
  {
      __set_errno (EINVAL);
      return NAN;
  }

  if (group)
    {
      grouping = _NL_CURRENT (LC_NUMERIC, GROUPING);
      if (*grouping <= 0 || *grouping == CHAR_MAX)
	grouping = NULL;
      else
	{
	  /* Figure out the thousands separator character.  */
#ifdef USE_WIDE_CHAR
	  thousands = _NL_CURRENT_WORD (LC_NUMERIC,
					_NL_NUMERIC_THOUSANDS_SEP_WC);
	  if (thousands == L'\0')
	    grouping = NULL;
#else
	  thousands = _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
	  if (*thousands == '\0')
	    {
	      thousands = NULL;
	      grouping = NULL;
	    }
#endif
	}
    }
  else
    grouping = NULL;

  /* Find the locale's decimal point character.  */
#ifdef USE_WIDE_CHAR
  decimal = _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_DECIMAL_POINT_WC);
  assert (decimal != L'\0');
# define decimal_len 1
#else
  decimal = _NL_CURRENT (LC_NUMERIC, DECIMAL_POINT);
  decimal_len = strlen (decimal);
  assert (decimal_len > 0);
#endif

  /* Prepare number representation.  */
  exponent = 0;
  negative = 0;
  bits = 0;

  /* Parse string to get maximal legal prefix.  We need the number of
     characters of the integer part, the fractional part and the exponent.  */
  cp = nptr - 1;
  /* Ignore leading white space.  */
  do
    c = *++cp;
  while (ISSPACE (c));

  /* Get sign of the result.  */
  if (c == L_('-'))
    {
      negative = 1;
      c = *++cp;
    }
  else if (c == L_('+'))
    c = *++cp;

  /* Return 0.0 if no legal string is found.
     No character is used even if a sign was found.  */
#ifdef USE_WIDE_CHAR
  if (c == decimal && cp[1] >= L'0' && cp[1] <= L'9')
    {
      /* We accept it.  This funny construct is here only to indent
	 the code directly.  */
    }
#else
  for (cnt = 0; decimal[cnt] != '\0'; ++cnt)
    if (cp[cnt] != decimal[cnt])
      break;
  if (decimal[cnt] == '\0' && cp[1] >= '0' && cp[1] <= '9')
    {
      /* We accept it.  This funny construct is here only to indent
	 the code directly.  */
    }
#endif
  else if (c < L_('0') || c > L_('9'))
    {
      /* Check for `INF' or `INFINITY'.  */
      if (TOLOWER (c) == L_('i') && STRNCASECMP (cp, L_("inf"), 3) == 0)
	{
	  /* Return +/- infinity.  */
	  if (endptr != NULL)
	    *endptr = (STRING_TYPE *)
		      (cp + (STRNCASECMP (cp + 3, L_("inity"), 5) == 0
			     ? 8 : 3));

	  return negative ? -FLOAT_HUGE_VAL : FLOAT_HUGE_VAL;
	}

      if (TOLOWER (c) == L_('n') && STRNCASECMP (cp, L_("nan"), 3) == 0)
	{
	  /* Return NaN.  */
	  FLOAT retval = NAN;

	  cp += 3;

	  /* Match `(n-char-sequence-digit)'.  */
	  if (*cp == L_('('))
	    {
	      const STRING_TYPE *startp = cp;
	      do
		++cp;
	      while ((*cp >= L_('0') && *cp <= L_('9'))
		     || (TOLOWER (*cp) >= L_('a') && TOLOWER (*cp) <= L_('z'))
		     || *cp == L_('_'));

	      if (*cp != L_(')'))
		/* The closing brace is missing.  Only match the NAN
		   part.  */
		cp = startp;
	      else
		{
		  /* This is a system-dependent way to specify the
		     bitmask used for the NaN.  We expect it to be
		     a number which is put in the mantissa of the
		     number.  */
		  STRING_TYPE *endp;
		  unsigned long long int mant;

		  mant = STRTOULL (startp + 1, &endp, 0);
		  if (endp == cp)
		    SET_MANTISSA (retval, mant);
		}
	    }

	  if (endptr != NULL)
	    *endptr = (STRING_TYPE *) cp;

	  return retval;
	}

      /* It is really a text we do not recognize.  */
      RETURN (0.0, nptr);
    }

  /* First look whether we are faced with a hexadecimal number.  */
  if (c == L_('0') && TOLOWER (cp[1]) == L_('x'))
    {
      /* Okay, it is a hexa-decimal number.  Remember this and skip
	 the characters.  BTW: hexadecimal numbers must not be
	 grouped.  */
      base = 16;
      cp += 2;
      c = *cp;
      grouping = NULL;
    }

  /* Record the start of the digits, in case we will check their grouping.  */
  start_of_digits = startp = cp;

  /* Ignore leading zeroes.  This helps us to avoid useless computations.  */
#ifdef USE_WIDE_CHAR
  while (c == L'0' || (thousands != L'\0' && c == thousands))
    c = *++cp;
#else
  if (thousands == NULL)
    while (c == '0')
      c = *++cp;
  else
    {
      /* We also have the multibyte thousands string.  */
      while (1)
	{
	  if (c != '0')
	    {
	      for (cnt = 0; thousands[cnt] != '\0'; ++cnt)
		if (c != thousands[cnt])
		  break;
	      if (thousands[cnt] != '\0')
		break;
	    }
	  c = *++cp;
	}
    }
#endif

  /* If no other digit but a '0' is found the result is 0.0.
     Return current read pointer.  */
  if (!((c >= L_('0') && c <= L_('9'))
	|| (base == 16 && ((CHAR_TYPE) TOLOWER (c) >= L_('a')
			   && (CHAR_TYPE) TOLOWER (c) <= L_('f')))
	|| (
#ifdef USE_WIDE_CHAR
	    c == (wint_t) decimal
#else
	    ({ for (cnt = 0; decimal[cnt] != '\0'; ++cnt)
		 if (decimal[cnt] != cp[cnt])
		   break;
	       decimal[cnt] == '\0'; })
#endif
	    /* '0x.' alone is not a valid hexadecimal number.
	       '.' alone is not valid either, but that has been checked
	       already earlier.  */
	    && (base != 16
		|| cp != start_of_digits
		|| (cp[decimal_len] >= L_('0') && cp[decimal_len] <= L_('9'))
		|| ((CHAR_TYPE) TOLOWER (cp[decimal_len]) >= L_('a')
		    && (CHAR_TYPE) TOLOWER (cp[decimal_len]) <= L_('f'))))
	|| (base == 16 && (cp != start_of_digits
			   && (CHAR_TYPE) TOLOWER (c) == L_('p')))
	|| (base != 16 && (CHAR_TYPE) TOLOWER (c) == L_('e'))))
    {
      tp = correctly_grouped_prefix (start_of_digits, cp, thousands, grouping);
      /* If TP is at the start of the digits, there was no correctly
	 grouped prefix of the string; so no number found.  */
      RETURN (0.0, tp == start_of_digits ? (base == 16 ? cp - 1 : nptr) : tp);
    }

  /* Remember first significant digit and read following characters until the
     decimal point, exponent character or any non-FP number character.  */
  startp = cp;
  dig_no = 0;
  while (1)
    {
      if ((c >= L_('0') && c <= L_('9'))
	  || (base == 16 && TOLOWER (c) >= L_('a') && TOLOWER (c) <= L_('f')))
	++dig_no;
      else
	{
#ifdef USE_WIDE_CHAR
	  if (thousands == L'\0' || c != thousands)
	    /* Not a digit or separator: end of the integer part.  */
	    break;
#else
	  if (thousands == NULL)
	    break;
	  else
	    {
	      for (cnt = 0; thousands[cnt] != '\0'; ++cnt)
		if (thousands[cnt] != cp[cnt])
		  break;
	      if (thousands[cnt] != '\0')
		break;
	    }
#endif
	}
      c = *++cp;
    }

  if (grouping && dig_no > 0)
    {
      /* Check the grouping of the digits.  */
      tp = correctly_grouped_prefix (start_of_digits, cp, thousands, grouping);
      if (cp != tp)
        {
	  /* Less than the entire string was correctly grouped.  */

	  if (tp == start_of_digits)
	    /* No valid group of numbers at all: no valid number.  */
	    RETURN (0.0, nptr);

	  if (tp < startp)
	    /* The number is validly grouped, but consists
	       only of zeroes.  The whole value is zero.  */
	    RETURN (0.0, tp);

	  /* Recompute DIG_NO so we won't read more digits than
	     are properly grouped.  */
	  cp = tp;
	  dig_no = 0;
	  for (tp = startp; tp < cp; ++tp)
	    if (*tp >= L_('0') && *tp <= L_('9'))
	      ++dig_no;

	  int_no = dig_no;
	  lead_zero = 0;

	  goto number_parsed;
	}
    }

  /* We have the number digits in the integer part.  Whether these are all or
     any is really a fractional digit will be decided later.  */
  int_no = dig_no;
  lead_zero = int_no == 0 ? -1 : 0;

  /* Read the fractional digits.  A special case are the 'american style'
     numbers like `16.' i.e. with decimal but without trailing digits.  */
  if (
#ifdef USE_WIDE_CHAR
      c == decimal
#else
      ({ for (cnt = 0; decimal[cnt] != '\0'; ++cnt)
	   if (decimal[cnt] != cp[cnt])
	     break;
	 decimal[cnt] == '\0'; })
#endif
      )
    {
      cp += decimal_len;
      c = *cp;
      while ((c >= L_('0') && c <= L_('9')) ||
	     (base == 16 && TOLOWER (c) >= L_('a') && TOLOWER (c) <= L_('f')))
	{
	  if (c != L_('0') && lead_zero == -1)
	    lead_zero = dig_no - int_no;
	  ++dig_no;
	  c = *++cp;
	}
    }

  /* Remember start of exponent (if any).  */
  expp = cp;

  /* Read exponent.  */
  if ((base == 16 && TOLOWER (c) == L_('p'))
      || (base != 16 && TOLOWER (c) == L_('e')))
    {
      int exp_negative = 0;

      c = *++cp;
      if (c == L_('-'))
	{
	  exp_negative = 1;
	  c = *++cp;
	}
      else if (c == L_('+'))
	c = *++cp;

      if (c >= L_('0') && c <= L_('9'))
	{
	  int exp_limit;

	  /* Get the exponent limit. */
	  if (base == 16)
	    exp_limit = (exp_negative ?
			 -MIN_EXP + MANT_DIG + 4 * int_no :
			 MAX_EXP - 4 * int_no + lead_zero);
	  else
	    exp_limit = (exp_negative ?
			 -MIN_10_EXP + MANT_DIG + int_no :
			 MAX_10_EXP - int_no + lead_zero);

	  do
	    {
	      exponent *= 10;

	      if (exponent > exp_limit)
		/* The exponent is too large/small to represent a valid
		   number.  */
		{
	 	  FLOAT result;

		  /* We have to take care for special situation: a joker
		     might have written "0.0e100000" which is in fact
		     zero.  */
		  if (lead_zero == -1)
		    result = negative ? -0.0 : 0.0;
		  else
		    {
		      /* Overflow or underflow.  */
		      __set_errno (ERANGE);
		      result = (exp_negative ? 0.0 :
				negative ? -FLOAT_HUGE_VAL : FLOAT_HUGE_VAL);
		    }

		  /* Accept all following digits as part of the exponent.  */
		  do
		    ++cp;
		  while (*cp >= L_('0') && *cp <= L_('9'));

		  RETURN (result, cp);
		  /* NOTREACHED */
		}

	      exponent += c - L_('0');
	      c = *++cp;
	    }
	  while (c >= L_('0') && c <= L_('9'));

	  if (exp_negative)
	    exponent = -exponent;
	}
      else
	cp = expp;
    }

  /* We don't want to have to work with trailing zeroes after the radix.  */
  if (dig_no > int_no)
    {
      while (expp[-1] == L_('0'))
	{
	  --expp;
	  --dig_no;
	}
      assert (dig_no >= int_no);
    }

  if (dig_no == int_no && dig_no > 0 && exponent < 0)
    do
      {
	while (expp[-1] < L_('0') || expp[-1] > L_('9'))
	  --expp;

	if (expp[-1] != L_('0'))
	  break;

	--expp;
	--dig_no;
	--int_no;
	++exponent;
      }
    while (dig_no > 0 && exponent < 0);

 number_parsed:

  /* The whole string is parsed.  Store the address of the next character.  */
  if (endptr)
    *endptr = (STRING_TYPE *) cp;

  if (dig_no == 0)
    return negative ? -0.0 : 0.0;

  if (lead_zero)
    {
      /* Find the decimal point */
#ifdef USE_WIDE_CHAR
      while (*startp != decimal)
	++startp;
#else
      while (1)
	{
	  if (*startp == decimal[0])
	    {
	      for (cnt = 1; decimal[cnt] != '\0'; ++cnt)
		if (decimal[cnt] != startp[cnt])
		  break;
	      if (decimal[cnt] == '\0')
		break;
	    }
	  ++startp;
	}
#endif
      startp += lead_zero + decimal_len;
      exponent -= base == 16 ? 4 * lead_zero : lead_zero;
      dig_no -= lead_zero;
    }

  /* If the BASE is 16 we can use a simpler algorithm.  */
  if (base == 16)
    {
      static const int nbits[16] = { 0, 1, 2, 2, 3, 3, 3, 3,
				     4, 4, 4, 4, 4, 4, 4, 4 };
      int idx = (MANT_DIG - 1) / BITS_PER_MP_LIMB;
      int pos = (MANT_DIG - 1) % BITS_PER_MP_LIMB;
      mp_limb_t val;

      while (!ISXDIGIT (*startp))
	++startp;
      while (*startp == L_('0'))
	++startp;
      if (ISDIGIT (*startp))
	val = *startp++ - L_('0');
      else
	val = 10 + TOLOWER (*startp++) - L_('a');
      bits = nbits[val];
      /* We cannot have a leading zero.  */
      assert (bits != 0);

      if (pos + 1 >= 4 || pos + 1 >= bits)
	{
	  /* We don't have to care for wrapping.  This is the normal
	     case so we add the first clause in the `if' expression as
	     an optimization.  It is a compile-time constant and so does
	     not cost anything.  */
	  retval[idx] = val << (pos - bits + 1);
	  pos -= bits;
	}
      else
	{
	  retval[idx--] = val >> (bits - pos - 1);
	  retval[idx] = val << (BITS_PER_MP_LIMB - (bits - pos - 1));
	  pos = BITS_PER_MP_LIMB - 1 - (bits - pos - 1);
	}

      /* Adjust the exponent for the bits we are shifting in.  */
      exponent += bits - 1 + (int_no - 1) * 4;

      while (--dig_no > 0 && idx >= 0)
	{
	  if (!ISXDIGIT (*startp))
	    startp += decimal_len;
	  if (ISDIGIT (*startp))
	    val = *startp++ - L_('0');
	  else
	    val = 10 + TOLOWER (*startp++) - L_('a');

	  if (pos + 1 >= 4)
	    {
	      retval[idx] |= val << (pos - 4 + 1);
	      pos -= 4;
	    }
	  else
	    {
	      retval[idx--] |= val >> (4 - pos - 1);
	      val <<= BITS_PER_MP_LIMB - (4 - pos - 1);
	      if (idx < 0)
		return round_and_return (retval, exponent, negative, val,
					 BITS_PER_MP_LIMB - 1, dig_no > 0);

	      retval[idx] = val;
	      pos = BITS_PER_MP_LIMB - 1 - (4 - pos - 1);
	    }
	}

      /* We ran out of digits.  */
      MPN_ZERO (retval, idx);

      return round_and_return (retval, exponent, negative, 0, 0, 0);
    }

  /* Now we have the number of digits in total and the integer digits as well
     as the exponent and its sign.  We can decide whether the read digits are
     really integer digits or belong to the fractional part; i.e. we normalize
     123e-2 to 1.23.  */
  {
    register int incr = (exponent < 0 ? MAX (-int_no, exponent)
			 : MIN (dig_no - int_no, exponent));
    int_no += incr;
    exponent -= incr;
  }

  if (int_no + exponent > MAX_10_EXP + 1)
    {
      __set_errno (ERANGE);
      return negative ? -FLOAT_HUGE_VAL : FLOAT_HUGE_VAL;
    }

  if (exponent < MIN_10_EXP - (DIG + 1))
    {
      __set_errno (ERANGE);
      return 0.0;
    }

  if (int_no > 0)
    {
      /* Read the integer part as a multi-precision number to NUM.  */
      startp = str_to_mpn (startp, int_no, num, &numsize, &exponent
#ifndef USE_WIDE_CHAR
			   , decimal, decimal_len, thousands
#endif
			   );

      if (exponent > 0)
	{
	  /* We now multiply the gained number by the given power of ten.  */
	  mp_limb_t *psrc = num;
	  mp_limb_t *pdest = den;
	  int expbit = 1;
	  const struct mp_power *ttab = &_fpioconst_pow10[0];

	  do
	    {
	      if ((exponent & expbit) != 0)
		{
		  size_t size = ttab->arraysize - _FPIO_CONST_OFFSET;
		  mp_limb_t cy;
		  exponent ^= expbit;

		  /* FIXME: not the whole multiplication has to be
		     done.  If we have the needed number of bits we
		     only need the information whether more non-zero
		     bits follow.  */
		  if (numsize >= ttab->arraysize - _FPIO_CONST_OFFSET)
		    cy = __mpn_mul (pdest, psrc, numsize,
				    &__tens[ttab->arrayoff
					   + _FPIO_CONST_OFFSET],
				    size);
		  else
		    cy = __mpn_mul (pdest, &__tens[ttab->arrayoff
						  + _FPIO_CONST_OFFSET],
				    size, psrc, numsize);
		  numsize += size;
		  if (cy == 0)
		    --numsize;
		  (void) SWAP (psrc, pdest);
		}
	      expbit <<= 1;
	      ++ttab;
	    }
	  while (exponent != 0);

	  if (psrc == den)
	    memcpy (num, den, numsize * sizeof (mp_limb_t));
	}

      /* Determine how many bits of the result we already have.  */
      count_leading_zeros (bits, num[numsize - 1]);
      bits = numsize * BITS_PER_MP_LIMB - bits;

      /* Now we know the exponent of the number in base two.
	 Check it against the maximum possible exponent.  */
      if (bits > MAX_EXP)
	{
	  __set_errno (ERANGE);
	  return negative ? -FLOAT_HUGE_VAL : FLOAT_HUGE_VAL;
	}

      /* We have already the first BITS bits of the result.  Together with
	 the information whether more non-zero bits follow this is enough
	 to determine the result.  */
      if (bits > MANT_DIG)
	{
	  int i;
	  const mp_size_t least_idx = (bits - MANT_DIG) / BITS_PER_MP_LIMB;
	  const mp_size_t least_bit = (bits - MANT_DIG) % BITS_PER_MP_LIMB;
	  const mp_size_t round_idx = least_bit == 0 ? least_idx - 1
						     : least_idx;
	  const mp_size_t round_bit = least_bit == 0 ? BITS_PER_MP_LIMB - 1
						     : least_bit - 1;

	  if (least_bit == 0)
	    memcpy (retval, &num[least_idx],
		    RETURN_LIMB_SIZE * sizeof (mp_limb_t));
	  else
            {
              for (i = least_idx; i < numsize - 1; ++i)
                retval[i - least_idx] = (num[i] >> least_bit)
                                        | (num[i + 1]
                                           << (BITS_PER_MP_LIMB - least_bit));
              if (i - least_idx < RETURN_LIMB_SIZE)
                retval[RETURN_LIMB_SIZE - 1] = num[i] >> least_bit;
            }

	  /* Check whether any limb beside the ones in RETVAL are non-zero.  */
	  for (i = 0; num[i] == 0; ++i)
	    ;

	  return round_and_return (retval, bits - 1, negative,
				   num[round_idx], round_bit,
				   int_no < dig_no || i < round_idx);
	  /* NOTREACHED */
	}
      else if (dig_no == int_no)
	{
	  const mp_size_t target_bit = (MANT_DIG - 1) % BITS_PER_MP_LIMB;
	  const mp_size_t is_bit = (bits - 1) % BITS_PER_MP_LIMB;

	  if (target_bit == is_bit)
	    {
	      memcpy (&retval[RETURN_LIMB_SIZE - numsize], num,
		      numsize * sizeof (mp_limb_t));
	      /* FIXME: the following loop can be avoided if we assume a
		 maximal MANT_DIG value.  */
	      MPN_ZERO (retval, RETURN_LIMB_SIZE - numsize);
	    }
	  else if (target_bit > is_bit)
	    {
	      (void) __mpn_lshift (&retval[RETURN_LIMB_SIZE - numsize],
				   num, numsize, target_bit - is_bit);
	      /* FIXME: the following loop can be avoided if we assume a
		 maximal MANT_DIG value.  */
	      MPN_ZERO (retval, RETURN_LIMB_SIZE - numsize);
	    }
	  else
	    {
	      mp_limb_t cy;
	      assert (numsize < RETURN_LIMB_SIZE);

	      cy = __mpn_rshift (&retval[RETURN_LIMB_SIZE - numsize],
				 num, numsize, is_bit - target_bit);
	      retval[RETURN_LIMB_SIZE - numsize - 1] = cy;
	      /* FIXME: the following loop can be avoided if we assume a
		 maximal MANT_DIG value.  */
	      MPN_ZERO (retval, RETURN_LIMB_SIZE - numsize - 1);
	    }

	  return round_and_return (retval, bits - 1, negative, 0, 0, 0);
	  /* NOTREACHED */
	}

      /* Store the bits we already have.  */
      memcpy (retval, num, numsize * sizeof (mp_limb_t));
#if RETURN_LIMB_SIZE > 1
      if (numsize < RETURN_LIMB_SIZE)
        retval[numsize] = 0;
#endif
    }

  /* We have to compute at least some of the fractional digits.  */
  {
    /* We construct a fraction and the result of the division gives us
       the needed digits.  The denominator is 1.0 multiplied by the
       exponent of the lowest digit; i.e. 0.123 gives 123 / 1000 and
       123e-6 gives 123 / 1000000.  */

    int expbit;
    int neg_exp;
    int more_bits;
    mp_limb_t cy;
    mp_limb_t *psrc = den;
    mp_limb_t *pdest = num;
    const struct mp_power *ttab = &_fpioconst_pow10[0];

    assert (dig_no > int_no && exponent <= 0);


    /* For the fractional part we need not process too many digits.  One
       decimal digits gives us log_2(10) ~ 3.32 bits.  If we now compute
                        ceil(BITS / 3) =: N
       digits we should have enough bits for the result.  The remaining
       decimal digits give us the information that more bits are following.
       This can be used while rounding.  (One added as a safety margin.)  */
    if (dig_no - int_no > (MANT_DIG - bits + 2) / 3 + 1)
      {
        dig_no = int_no + (MANT_DIG - bits + 2) / 3 + 1;
        more_bits = 1;
      }
    else
      more_bits = 0;

    neg_exp = dig_no - int_no - exponent;

    /* Construct the denominator.  */
    densize = 0;
    expbit = 1;
    do
      {
	if ((neg_exp & expbit) != 0)
	  {
	    mp_limb_t cy;
	    neg_exp ^= expbit;

	    if (densize == 0)
	      {
		densize = ttab->arraysize - _FPIO_CONST_OFFSET;
		memcpy (psrc, &__tens[ttab->arrayoff + _FPIO_CONST_OFFSET],
			densize * sizeof (mp_limb_t));
	      }
	    else
	      {
		cy = __mpn_mul (pdest, &__tens[ttab->arrayoff
					      + _FPIO_CONST_OFFSET],
				ttab->arraysize - _FPIO_CONST_OFFSET,
				psrc, densize);
		densize += ttab->arraysize - _FPIO_CONST_OFFSET;
		if (cy == 0)
		  --densize;
		(void) SWAP (psrc, pdest);
	      }
	  }
	expbit <<= 1;
	++ttab;
      }
    while (neg_exp != 0);

    if (psrc == num)
      memcpy (den, num, densize * sizeof (mp_limb_t));

    /* Read the fractional digits from the string.  */
    (void) str_to_mpn (startp, dig_no - int_no, num, &numsize, &exponent
#ifndef USE_WIDE_CHAR
		       , decimal, decimal_len, thousands
#endif
		       );

    /* We now have to shift both numbers so that the highest bit in the
       denominator is set.  In the same process we copy the numerator to
       a high place in the array so that the division constructs the wanted
       digits.  This is done by a "quasi fix point" number representation.

       num:   ddddddddddd . 0000000000000000000000
              |--- m ---|
       den:                            ddddddddddd      n >= m
                                       |--- n ---|
     */

    count_leading_zeros (cnt, den[densize - 1]);

    if (cnt > 0)
      {
	/* Don't call `mpn_shift' with a count of zero since the specification
	   does not allow this.  */
	(void) __mpn_lshift (den, den, densize, cnt);
	cy = __mpn_lshift (num, num, numsize, cnt);
	if (cy != 0)
	  num[numsize++] = cy;
      }

    /* Now we are ready for the division.  But it is not necessary to
       do a full multi-precision division because we only need a small
       number of bits for the result.  So we do not use __mpn_divmod
       here but instead do the division here by hand and stop whenever
       the needed number of bits is reached.  The code itself comes
       from the GNU MP Library by Torbj\"orn Granlund.  */

    exponent = bits;

    switch (densize)
      {
      case 1:
	{
	  mp_limb_t d, n, quot;
	  int used = 0;

	  n = num[0];
	  d = den[0];
	  assert (numsize == 1 && n < d);

	  do
	    {
	      udiv_qrnnd (quot, n, n, 0, d);

#define got_limb							      \
	      if (bits == 0)						      \
		{							      \
		  register int cnt;					      \
		  if (quot == 0)					      \
		    cnt = BITS_PER_MP_LIMB;				      \
		  else							      \
		    count_leading_zeros (cnt, quot);			      \
		  exponent -= cnt;					      \
		  if (BITS_PER_MP_LIMB - cnt > MANT_DIG)		      \
		    {							      \
		      used = MANT_DIG + cnt;				      \
		      retval[0] = quot >> (BITS_PER_MP_LIMB - used);	      \
		      bits = MANT_DIG + 1;				      \
		    }							      \
		  else							      \
		    {							      \
		      /* Note that we only clear the second element.  */      \
		      /* The conditional is determined at compile time.  */   \
		      if (RETURN_LIMB_SIZE > 1)				      \
			retval[1] = 0;					      \
		      retval[0] = quot;					      \
		      bits = -cnt;					      \
		    }							      \
		}							      \
	      else if (bits + BITS_PER_MP_LIMB <= MANT_DIG)		      \
		__mpn_lshift_1 (retval, RETURN_LIMB_SIZE, BITS_PER_MP_LIMB,   \
				quot);					      \
	      else							      \
		{							      \
		  used = MANT_DIG - bits;				      \
		  if (used > 0)						      \
		    __mpn_lshift_1 (retval, RETURN_LIMB_SIZE, used, quot);    \
		}							      \
	      bits += BITS_PER_MP_LIMB

	      got_limb;
	    }
	  while (bits <= MANT_DIG);

	  return round_and_return (retval, exponent - 1, negative,
				   quot, BITS_PER_MP_LIMB - 1 - used,
				   more_bits || n != 0);
	}
      case 2:
	{
	  mp_limb_t d0, d1, n0, n1;
	  mp_limb_t quot = 0;
	  int used = 0;

	  d0 = den[0];
	  d1 = den[1];

	  if (numsize < densize)
	    {
	      if (num[0] >= d1)
		{
		  /* The numerator of the number occupies fewer bits than
		     the denominator but the one limb is bigger than the
		     high limb of the numerator.  */
		  n1 = 0;
		  n0 = num[0];
		}
	      else
		{
		  if (bits <= 0)
		    exponent -= BITS_PER_MP_LIMB;
		  else
		    {
		      if (bits + BITS_PER_MP_LIMB <= MANT_DIG)
			__mpn_lshift_1 (retval, RETURN_LIMB_SIZE,
					BITS_PER_MP_LIMB, 0);
		      else
			{
			  used = MANT_DIG - bits;
			  if (used > 0)
			    __mpn_lshift_1 (retval, RETURN_LIMB_SIZE, used, 0);
			}
		      bits += BITS_PER_MP_LIMB;
		    }
		  n1 = num[0];
		  n0 = 0;
		}
	    }
	  else
	    {
	      n1 = num[1];
	      n0 = num[0];
	    }

	  while (bits <= MANT_DIG)
	    {
	      mp_limb_t r;

	      if (n1 == d1)
		{
		  /* QUOT should be either 111..111 or 111..110.  We need
		     special treatment of this rare case as normal division
		     would give overflow.  */
		  quot = ~(mp_limb_t) 0;

		  r = n0 + d1;
		  if (r < d1)	/* Carry in the addition?  */
		    {
		      add_ssaaaa (n1, n0, r - d0, 0, 0, d0);
		      goto have_quot;
		    }
		  n1 = d0 - (d0 != 0);
		  n0 = -d0;
		}
	      else
		{
		  udiv_qrnnd (quot, r, n1, n0, d1);
		  umul_ppmm (n1, n0, d0, quot);
		}

	    q_test:
	      if (n1 > r || (n1 == r && n0 > 0))
		{
		  /* The estimated QUOT was too large.  */
		  --quot;

		  sub_ddmmss (n1, n0, n1, n0, 0, d0);
		  r += d1;
		  if (r >= d1)	/* If not carry, test QUOT again.  */
		    goto q_test;
		}
	      sub_ddmmss (n1, n0, r, 0, n1, n0);

	    have_quot:
	      got_limb;
	    }

	  return round_and_return (retval, exponent - 1, negative,
				   quot, BITS_PER_MP_LIMB - 1 - used,
				   more_bits || n1 != 0 || n0 != 0);
	}
      default:
	{
	  int i;
	  mp_limb_t cy, dX, d1, n0, n1;
	  mp_limb_t quot = 0;
	  int used = 0;

	  dX = den[densize - 1];
	  d1 = den[densize - 2];

	  /* The division does not work if the upper limb of the two-limb
	     numerator is greater than the denominator.  */
	  if (__mpn_cmp (num, &den[densize - numsize], numsize) > 0)
	    num[numsize++] = 0;

	  if (numsize < densize)
	    {
	      mp_size_t empty = densize - numsize;

	      if (bits <= 0)
		{
		  register int i;
		  for (i = numsize; i > 0; --i)
		    num[i + empty] = num[i - 1];
		  MPN_ZERO (num, empty + 1);
		  exponent -= empty * BITS_PER_MP_LIMB;
		}
	      else
		{
		  if (bits + empty * BITS_PER_MP_LIMB <= MANT_DIG)
		    {
		      /* We make a difference here because the compiler
			 cannot optimize the `else' case that good and
			 this reflects all currently used FLOAT types
			 and GMP implementations.  */
		      register int i;
#if RETURN_LIMB_SIZE <= 2
		      assert (empty == 1);
		      __mpn_lshift_1 (retval, RETURN_LIMB_SIZE,
				      BITS_PER_MP_LIMB, 0);
#else
		      for (i = RETURN_LIMB_SIZE; i > empty; --i)
			retval[i] = retval[i - empty];
#endif
#if RETURN_LIMB_SIZE > 1
		      retval[1] = 0;
#endif
		      for (i = numsize; i > 0; --i)
			num[i + empty] = num[i - 1];
		      MPN_ZERO (num, empty + 1);
		    }
		  else
		    {
		      used = MANT_DIG - bits;
		      if (used >= BITS_PER_MP_LIMB)
			{
			  register int i;
			  (void) __mpn_lshift (&retval[used
						       / BITS_PER_MP_LIMB],
					       retval, RETURN_LIMB_SIZE,
					       used % BITS_PER_MP_LIMB);
			  for (i = used / BITS_PER_MP_LIMB; i >= 0; --i)
			    retval[i] = 0;
			}
		      else if (used > 0)
			__mpn_lshift_1 (retval, RETURN_LIMB_SIZE, used, 0);
		    }
		  bits += empty * BITS_PER_MP_LIMB;
		}
	    }
	  else
	    {
	      int i;
	      assert (numsize == densize);
	      for (i = numsize; i > 0; --i)
		num[i] = num[i - 1];
	    }

	  den[densize] = 0;
	  n0 = num[densize];

	  while (bits <= MANT_DIG)
	    {
	      if (n0 == dX)
		/* This might over-estimate QUOT, but it's probably not
		   worth the extra code here to find out.  */
		quot = ~(mp_limb_t) 0;
	      else
		{
		  mp_limb_t r;

		  udiv_qrnnd (quot, r, n0, num[densize - 1], dX);
		  umul_ppmm (n1, n0, d1, quot);

		  while (n1 > r || (n1 == r && n0 > num[densize - 2]))
		    {
		      --quot;
		      r += dX;
		      if (r < dX) /* I.e. "carry in previous addition?" */
			break;
		      n1 -= n0 < d1;
		      n0 -= d1;
		    }
		}

	      /* Possible optimization: We already have (q * n0) and (1 * n1)
		 after the calculation of QUOT.  Taking advantage of this, we
		 could make this loop make two iterations less.  */

	      cy = __mpn_submul_1 (num, den, densize + 1, quot);

	      if (num[densize] != cy)
		{
		  cy = __mpn_add_n (num, num, den, densize);
		  assert (cy != 0);
		  --quot;
		}
	      n0 = num[densize] = num[densize - 1];
	      for (i = densize - 1; i > 0; --i)
		num[i] = num[i - 1];

	      got_limb;
	    }

	  for (i = densize; num[i] == 0 && i >= 0; --i)
	    ;
	  return round_and_return (retval, exponent - 1, negative,
				   quot, BITS_PER_MP_LIMB - 1 - used,
				   more_bits || i >= 0);
	}
      }
  }

  /* NOTREACHED */
}

/* External user entry point.  */

FLOAT
#ifdef weak_function
weak_function
#endif
STRTOF (nptr, endptr LOCALE_PARAM)
     const STRING_TYPE *nptr;
     STRING_TYPE **endptr;
     LOCALE_PARAM_DECL
{
  return INTERNAL (STRTOF) (nptr, endptr, 0 LOCALE_PARAM);
}
