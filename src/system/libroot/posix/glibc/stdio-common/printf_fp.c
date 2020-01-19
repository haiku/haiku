/* Floating point output for `printf'.
   Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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

/* The gmp headers need some configuration frobs.  */
#define HAVE_ALLOCA 1

#ifdef USE_IN_LIBIO
#  include <libioP.h>
#else
#  include <stdio.h>
#endif
#include <alloca.h>
#include <ctype.h>
#include <float.h>
#include <gmp-mparam.h>
#include <gmp.h>
#include <stdlib/gmp-impl.h>
#include <stdlib/longlong.h>
#include <stdlib/fpioconst.h>
#include <locale/localeinfo.h>
#include <limits.h>
#include <math.h>
#include <printf.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <wchar.h>

#ifndef NDEBUG
# define NDEBUG			/* Undefine this for debugging assertions.  */
#endif
#include <assert.h>

/* This defines make it possible to use the same code for GNU C library and
   the GNU I/O library.	 */
#ifdef USE_IN_LIBIO
# define PUT(f, s, n) _IO_sputn (f, s, n)
# define PAD(f, c, n) (wide ? _IO_wpadn (f, c, n) : _IO_padn (f, c, n))
/* We use this file GNU C library and GNU I/O library.	So make
   names equal.	 */
# undef putc
# define putc(c, f) (wide \
		      ? (int)_IO_putwc_unlocked (c, f) : _IO_putc_unlocked (c, f))
# define size_t     _IO_size_t
# define FILE	     _IO_FILE
#else	/* ! USE_IN_LIBIO */
# define PUT(f, s, n) fwrite (s, 1, n, f)
# define PAD(f, c, n) __printf_pad (f, c, n)
ssize_t __printf_pad __P ((FILE *, char pad, int n)); /* In vfprintf.c.  */
#endif	/* USE_IN_LIBIO */

/* Macros for doing the actual output.  */

#define outchar(ch)							      \
  do									      \
    {									      \
      register const int outc = (ch);					      \
      if (putc (outc, fp) == EOF)					      \
	return -1;							      \
      ++done;								      \
    } while (0)

#define PRINT(ptr, wptr, len)						      \
  do									      \
    {									      \
      register size_t outlen = (len);					      \
      if (len > 20)							      \
	{								      \
	  if (PUT (fp, wide ? (const char *) wptr : ptr, outlen) != outlen)   \
	    return -1;							      \
	  ptr += outlen;						      \
	  done += outlen;						      \
	}								      \
      else								      \
	{								      \
	  if (wide)							      \
	    while (outlen-- > 0)					      \
	      outchar (*wptr++);					      \
	  else								      \
	    while (outlen-- > 0)					      \
	      outchar (*ptr++);						      \
	}								      \
    } while (0)

#define PADN(ch, len)							      \
  do									      \
    {									      \
      if (PAD (fp, ch, len) != len)					      \
	return -1;							      \
      done += len;							      \
    }									      \
  while (0)

/* We use the GNU MP library to handle large numbers.

   An MP variable occupies a varying number of entries in its array.  We keep
   track of this number for efficiency reasons.  Otherwise we would always
   have to process the whole array.  */
#define MPN_VAR(name) mp_limb_t *name; mp_size_t name##size

#define MPN_ASSIGN(dst,src)						      \
  memcpy (dst, src, (dst##size = src##size) * sizeof (mp_limb_t))
#define MPN_GE(u,v) \
  (u##size > v##size || (u##size == v##size && __mpn_cmp (u, v, u##size) >= 0))

extern mp_size_t __mpn_extract_double (mp_ptr res_ptr, mp_size_t size,
				       int *expt, int *is_neg,
				       double value);
extern mp_size_t __mpn_extract_long_double (mp_ptr res_ptr, mp_size_t size,
					    int *expt, int *is_neg,
					    long double value);
extern unsigned int __guess_grouping (unsigned int intdig_max,
				      const char *grouping);


static wchar_t *group_number (wchar_t *buf, wchar_t *bufend,
			      unsigned int intdig_no, const char *grouping,
			      wchar_t thousands_sep, int ngroups)
     internal_function;


int
__printf_fp (FILE *fp,
	     const struct printf_info *info,
	     const void *const *args)
{
  /* The floating-point value to output.  */
  union
    {
      double dbl;
      __long_double_t ldbl;
    }
  fpnum;

  /* Locale-dependent representation of decimal point.	*/
  const char *decimal;
  wchar_t decimalwc;

  /* Locale-dependent thousands separator and grouping specification.  */
  const char *thousands_sep = NULL;
  wchar_t thousands_sepwc = 0;
  const char *grouping;

  /* "NaN" or "Inf" for the special cases.  */
  const char *special = NULL;
  const wchar_t *wspecial = NULL;

  /* We need just a few limbs for the input before shifting to the right
     position.	*/
  mp_limb_t fp_input[(LDBL_MANT_DIG + BITS_PER_MP_LIMB - 1) / BITS_PER_MP_LIMB];
  /* We need to shift the contents of fp_input by this amount of bits.	*/
  int to_shift = 0;

  /* The fraction of the floting-point value in question  */
  MPN_VAR(frac);
  /* and the exponent.	*/
  int exponent;
  /* Sign of the exponent.  */
  int expsign = 0;
  /* Sign of float number.  */
  int is_neg = 0;

  /* Scaling factor.  */
  MPN_VAR(scale);

  /* Temporary bignum value.  */
  MPN_VAR(tmp);

  /* Digit which is result of last hack_digit() call.  */
  wchar_t digit;

  /* The type of output format that will be used: 'e'/'E' or 'f'.  */
  int type;

  /* Counter for number of written characters.	*/
  int done = 0;

  /* General helper (carry limb).  */
  mp_limb_t cy;

  /* Nonzero if this is output on a wide character stream.  */
  int wide = info->wide;

  wchar_t hack_digit_ret;
  int hack_digit_callee;

  while (0)
    {
      mp_limb_t hi;

hack_digit:
      if (expsign != 0 && type == 'f' && exponent-- > 0)
	hi = 0;
      else if (scalesize == 0)
	{
	  hi = frac[fracsize - 1];
	  cy = __mpn_mul_1 (frac, frac, fracsize - 1, 10);
	  frac[fracsize - 1] = cy;
	}
      else
	{
	  if (fracsize < scalesize)
	    hi = 0;
	  else
	    {
	      hi = mpn_divmod (tmp, frac, fracsize, scale, scalesize);
	      tmp[fracsize - scalesize] = hi;
	      hi = tmp[0];

	      fracsize = scalesize;
	      while (fracsize != 0 && frac[fracsize - 1] == 0)
		--fracsize;
	      if (fracsize == 0)
		{
		  /* We're not prepared for an mpn variable with zero
		     limbs.  */
		  fracsize = 1;
		  hack_digit_ret = L'0' + hi;
		  goto hack_digit_end;
		}
	    }

	  cy = __mpn_mul_1 (frac, frac, fracsize, 10);
	  if (cy != 0)
	    frac[fracsize++] = cy;
	}

      hack_digit_ret = L'0' + hi;
hack_digit_end:
      switch (hack_digit_callee)
        {
	  case 1: goto hack_digit_callee1;
	  case 2: goto hack_digit_callee2;
	  case 3: goto hack_digit_callee3;
	  default: abort();
	}
    }


  /* Figure out the decimal point character.  */
  if (info->extra == 0)
    {
      decimal = _NL_CURRENT (LC_NUMERIC, DECIMAL_POINT);
      decimalwc = _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_DECIMAL_POINT_WC);
    }
  else
    {
      decimal = _NL_CURRENT (LC_MONETARY, MON_DECIMAL_POINT);
      if (*decimal == '\0')
	decimal = _NL_CURRENT (LC_NUMERIC, DECIMAL_POINT);
      decimalwc = _NL_CURRENT_WORD (LC_MONETARY,
				    _NL_MONETARY_DECIMAL_POINT_WC);
      if (decimalwc == L'\0')
	decimalwc = _NL_CURRENT_WORD (LC_NUMERIC,
				      _NL_NUMERIC_DECIMAL_POINT_WC);
    }
  /* The decimal point character must not be zero.  */
  assert (*decimal != '\0');
  assert (decimalwc != L'\0');

  if (info->group)
    {
      if (info->extra == 0)
	grouping = _NL_CURRENT (LC_NUMERIC, GROUPING);
      else
	grouping = _NL_CURRENT (LC_MONETARY, MON_GROUPING);

      if (*grouping <= 0 || *grouping == CHAR_MAX)
	grouping = NULL;
      else
	{
	  /* Figure out the thousands separator character.  */
	  if (wide)
	    {
	      if (info->extra == 0)
		thousands_sepwc =
		  _NL_CURRENT_WORD (LC_NUMERIC, _NL_NUMERIC_THOUSANDS_SEP_WC);
	      else
		thousands_sepwc =
		  _NL_CURRENT_WORD (LC_MONETARY,
				    _NL_MONETARY_THOUSANDS_SEP_WC);
	    }
	  else
	    {
	      if (info->extra == 0)
		thousands_sep = _NL_CURRENT (LC_NUMERIC, THOUSANDS_SEP);
	      else
		thousands_sep = _NL_CURRENT (LC_MONETARY, MON_THOUSANDS_SEP);
	    }

	  if ((wide && thousands_sepwc == L'\0')
	      || (! wide && *thousands_sep == '\0'))
	    grouping = NULL;
	  else if (thousands_sepwc == L'\0')
	    /* If we are printing multibyte characters and there is a
	       multibyte representation for the thousands separator,
	       we must ensure the wide character thousands separator
	       is available, even if it is fake.  */
	    thousands_sepwc = 0xfffffffe;
	}
    }
  else
    grouping = NULL;

  /* Fetch the argument value.	*/
#ifndef __NO_LONG_DOUBLE_MATH
  if (info->is_long_double && sizeof (long double) > sizeof (double))
    {
      fpnum.ldbl = *(const long double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (isnan (fpnum.ldbl))
	{
	  if (isupper (info->spec))
	    {
	      special = "NAN";
	      wspecial = L"NAN";
	    }
	    else
	      {
		special = "nan";
		wspecial = L"nan";
	      }
	  is_neg = 0;
	}
      else if (isinf (fpnum.ldbl))
	{
	  if (isupper (info->spec))
	    {
	      special = "INF";
	      wspecial = L"INF";
	    }
	  else
	    {
	      special = "inf";
	      wspecial = L"inf";
	    }
	  is_neg = fpnum.ldbl < 0;
	}
      else
	{
	  fracsize = __mpn_extract_long_double (fp_input,
						(sizeof (fp_input) /
						 sizeof (fp_input[0])),
						&exponent, &is_neg,
						fpnum.ldbl);
	  to_shift = 1 + fracsize * BITS_PER_MP_LIMB - LDBL_MANT_DIG;
	}
    }
  else
#endif	/* no long double */
    {
      fpnum.dbl = *(const double *) args[0];

      /* Check for special values: not a number or infinity.  */
      if (isnan (fpnum.dbl))
	{
	  if (isupper (info->spec))
	    {
	      special = "NAN";
	      wspecial = L"NAN";
	    }
	  else
	    {
	      special = "nan";
	      wspecial = L"nan";
	    }
	  is_neg = 0;
	}
      else if (isinf (fpnum.dbl))
	{
	  if (isupper (info->spec))
	    {
	      special = "INF";
	      wspecial = L"INF";
	    }
	  else
	    {
	      special = "inf";
	      wspecial = L"inf";
	    }
	  is_neg = fpnum.dbl < 0;
	}
      else
	{
	  fracsize = __mpn_extract_double (fp_input,
					   (sizeof (fp_input)
					    / sizeof (fp_input[0])),
					   &exponent, &is_neg, fpnum.dbl);
	  to_shift = 1 + fracsize * BITS_PER_MP_LIMB - DBL_MANT_DIG;
	}
    }

  if (special)
    {
      int width = info->width;

      if (is_neg || info->showsign || info->space)
	--width;
      width -= 3;

      if (!info->left && width > 0)
	PADN (' ', width);

      if (is_neg)
	outchar ('-');
      else if (info->showsign)
	outchar ('+');
      else if (info->space)
	outchar (' ');

      PRINT (special, wspecial, 3);

      if (info->left && width > 0)
	PADN (' ', width);

      return done;
    }


  /* We need three multiprecision variables.  Now that we have the exponent
     of the number we can allocate the needed memory.  It would be more
     efficient to use variables of the fixed maximum size but because this
     would be really big it could lead to memory problems.  */
  {
    mp_size_t bignum_size = ((ABS (exponent) + BITS_PER_MP_LIMB - 1)
			     / BITS_PER_MP_LIMB + 4) * sizeof (mp_limb_t);
    frac = (mp_limb_t *) alloca (bignum_size);
    tmp = (mp_limb_t *) alloca (bignum_size);
    scale = (mp_limb_t *) alloca (bignum_size);
  }

  /* We now have to distinguish between numbers with positive and negative
     exponents because the method used for the one is not applicable/efficient
     for the other.  */
  scalesize = 0;
  if (exponent > 2)
    {
      /* |FP| >= 8.0.  */
      int scaleexpo = 0;
      int explog = LDBL_MAX_10_EXP_LOG;
      int exp10 = 0;
      const struct mp_power *powers = &_fpioconst_pow10[explog + 1];
      int cnt_h, cnt_l, i;

      if ((exponent + to_shift) % BITS_PER_MP_LIMB == 0)
	{
	  MPN_COPY_DECR (frac + (exponent + to_shift) / BITS_PER_MP_LIMB,
			 fp_input, fracsize);
	  fracsize += (exponent + to_shift) / BITS_PER_MP_LIMB;
	}
      else
	{
	  cy = __mpn_lshift (frac + (exponent + to_shift) / BITS_PER_MP_LIMB,
			     fp_input, fracsize,
			     (exponent + to_shift) % BITS_PER_MP_LIMB);
	  fracsize += (exponent + to_shift) / BITS_PER_MP_LIMB;
	  if (cy)
	    frac[fracsize++] = cy;
	}
      MPN_ZERO (frac, (exponent + to_shift) / BITS_PER_MP_LIMB);

      assert (powers > &_fpioconst_pow10[0]);
      do
	{
	  --powers;

	  /* The number of the product of two binary numbers with n and m
	     bits respectively has m+n or m+n-1 bits.	*/
	  if (exponent >= scaleexpo + powers->p_expo - 1)
	    {
	      if (scalesize == 0)
		{
#ifndef __NO_LONG_DOUBLE_MATH
		  if (LDBL_MANT_DIG > _FPIO_CONST_OFFSET * BITS_PER_MP_LIMB
		      && info->is_long_double)
		    {
#define _FPIO_CONST_SHIFT \
  (((LDBL_MANT_DIG + BITS_PER_MP_LIMB - 1) / BITS_PER_MP_LIMB) \
   - _FPIO_CONST_OFFSET)
		      /* 64bit const offset is not enough for
			 IEEE quad long double.  */
		      tmpsize = powers->arraysize + _FPIO_CONST_SHIFT;
		      memcpy (tmp + _FPIO_CONST_SHIFT,
			      &__tens[powers->arrayoff],
			      tmpsize * sizeof (mp_limb_t));
		      MPN_ZERO (tmp, _FPIO_CONST_SHIFT);
		    }
		  else
#endif
		    {
		      tmpsize = powers->arraysize;
		      memcpy (tmp, &__tens[powers->arrayoff],
			      tmpsize * sizeof (mp_limb_t));
		    }
		}
	      else
		{
		  cy = __mpn_mul (tmp, scale, scalesize,
				  &__tens[powers->arrayoff
					 + _FPIO_CONST_OFFSET],
				  powers->arraysize - _FPIO_CONST_OFFSET);
		  tmpsize = scalesize + powers->arraysize - _FPIO_CONST_OFFSET;
		  if (cy == 0)
		    --tmpsize;
		}

	      if (MPN_GE (frac, tmp))
		{
		  int cnt;
		  MPN_ASSIGN (scale, tmp);
		  count_leading_zeros (cnt, scale[scalesize - 1]);
		  scaleexpo = (scalesize - 2) * BITS_PER_MP_LIMB - cnt - 1;
		  exp10 |= 1 << explog;
		}
	    }
	  --explog;
	}
      while (powers > &_fpioconst_pow10[0]);
      exponent = exp10;

      /* Optimize number representations.  We want to represent the numbers
	 with the lowest number of bytes possible without losing any
	 bytes. Also the highest bit in the scaling factor has to be set
	 (this is a requirement of the MPN division routines).  */
      if (scalesize > 0)
	{
	  /* Determine minimum number of zero bits at the end of
	     both numbers.  */
	  for (i = 0; scale[i] == 0 && frac[i] == 0; i++)
	    ;

	  /* Determine number of bits the scaling factor is misplaced.	*/
	  count_leading_zeros (cnt_h, scale[scalesize - 1]);

	  if (cnt_h == 0)
	    {
	      /* The highest bit of the scaling factor is already set.	So
		 we only have to remove the trailing empty limbs.  */
	      if (i > 0)
		{
		  MPN_COPY_INCR (scale, scale + i, scalesize - i);
		  scalesize -= i;
		  MPN_COPY_INCR (frac, frac + i, fracsize - i);
		  fracsize -= i;
		}
	    }
	  else
	    {
	      if (scale[i] != 0)
		{
		  count_trailing_zeros (cnt_l, scale[i]);
		  if (frac[i] != 0)
		    {
		      int cnt_l2;
		      count_trailing_zeros (cnt_l2, frac[i]);
		      if (cnt_l2 < cnt_l)
			cnt_l = cnt_l2;
		    }
		}
	      else
		count_trailing_zeros (cnt_l, frac[i]);

	      /* Now shift the numbers to their optimal position.  */
	      if (i == 0 && BITS_PER_MP_LIMB - cnt_h > cnt_l)
		{
		  /* We cannot save any memory.	 So just roll both numbers
		     so that the scaling factor has its highest bit set.  */

		  (void) __mpn_lshift (scale, scale, scalesize, cnt_h);
		  cy = __mpn_lshift (frac, frac, fracsize, cnt_h);
		  if (cy != 0)
		    frac[fracsize++] = cy;
		}
	      else if (BITS_PER_MP_LIMB - cnt_h <= cnt_l)
		{
		  /* We can save memory by removing the trailing zero limbs
		     and by packing the non-zero limbs which gain another
		     free one. */

		  (void) __mpn_rshift (scale, scale + i, scalesize - i,
				       BITS_PER_MP_LIMB - cnt_h);
		  scalesize -= i + 1;
		  (void) __mpn_rshift (frac, frac + i, fracsize - i,
				       BITS_PER_MP_LIMB - cnt_h);
		  fracsize -= frac[fracsize - i - 1] == 0 ? i + 1 : i;
		}
	      else
		{
		  /* We can only save the memory of the limbs which are zero.
		     The non-zero parts occupy the same number of limbs.  */

		  (void) __mpn_rshift (scale, scale + (i - 1),
				       scalesize - (i - 1),
				       BITS_PER_MP_LIMB - cnt_h);
		  scalesize -= i;
		  (void) __mpn_rshift (frac, frac + (i - 1),
				       fracsize - (i - 1),
				       BITS_PER_MP_LIMB - cnt_h);
		  fracsize -= frac[fracsize - (i - 1) - 1] == 0 ? i : i - 1;
		}
	    }
	}
    }
  else if (exponent < 0)
    {
      /* |FP| < 1.0.  */
      int exp10 = 0;
      int explog = LDBL_MAX_10_EXP_LOG;
      const struct mp_power *powers = &_fpioconst_pow10[explog + 1];
      mp_size_t used_limbs = fracsize - 1;

      /* Now shift the input value to its right place.	*/
      cy = __mpn_lshift (frac, fp_input, fracsize, to_shift);
      frac[fracsize++] = cy;
      assert (cy == 1 || (frac[fracsize - 2] == 0 && frac[0] == 0));

      expsign = 1;
      exponent = -exponent;

      assert (powers != &_fpioconst_pow10[0]);
      do
	{
	  --powers;

	  if (exponent >= powers->m_expo)
	    {
	      int i, incr, cnt_h, cnt_l;
	      mp_limb_t topval[2];

	      /* The __mpn_mul function expects the first argument to be
		 bigger than the second.  */
	      if (fracsize < powers->arraysize - _FPIO_CONST_OFFSET)
		cy = __mpn_mul (tmp, &__tens[powers->arrayoff
					    + _FPIO_CONST_OFFSET],
				powers->arraysize - _FPIO_CONST_OFFSET,
				frac, fracsize);
	      else
		cy = __mpn_mul (tmp, frac, fracsize,
				&__tens[powers->arrayoff + _FPIO_CONST_OFFSET],
				powers->arraysize - _FPIO_CONST_OFFSET);
	      tmpsize = fracsize + powers->arraysize - _FPIO_CONST_OFFSET;
	      if (cy == 0)
		--tmpsize;

	      count_leading_zeros (cnt_h, tmp[tmpsize - 1]);
	      incr = (tmpsize - fracsize) * BITS_PER_MP_LIMB
		     + BITS_PER_MP_LIMB - 1 - cnt_h;

	      assert (incr <= powers->p_expo);

	      /* If we increased the exponent by exactly 3 we have to test
		 for overflow.	This is done by comparing with 10 shifted
		 to the right position.	 */
	      if (incr == exponent + 3)
		{
		  if (cnt_h <= BITS_PER_MP_LIMB - 4)
		    {
		      topval[0] = 0;
		      topval[1]
			= ((mp_limb_t) 10) << (BITS_PER_MP_LIMB - 4 - cnt_h);
		    }
		  else
		    {
		      topval[0] = ((mp_limb_t) 10) << (BITS_PER_MP_LIMB - 4);
		      topval[1] = 0;
		      (void) __mpn_lshift (topval, topval, 2,
					   BITS_PER_MP_LIMB - cnt_h);
		    }
		}

	      /* We have to be careful when multiplying the last factor.
		 If the result is greater than 1.0 be have to test it
		 against 10.0.  If it is greater or equal to 10.0 the
		 multiplication was not valid.  This is because we cannot
		 determine the number of bits in the result in advance.  */
	      if (incr < exponent + 3
		  || (incr == exponent + 3 &&
		      (tmp[tmpsize - 1] < topval[1]
		       || (tmp[tmpsize - 1] == topval[1]
			   && tmp[tmpsize - 2] < topval[0]))))
		{
		  /* The factor is right.  Adapt binary and decimal
		     exponents.	 */
		  exponent -= incr;
		  exp10 |= 1 << explog;

		  /* If this factor yields a number greater or equal to
		     1.0, we must not shift the non-fractional digits down. */
		  if (exponent < 0)
		    cnt_h += -exponent;

		  /* Now we optimize the number representation.	 */
		  for (i = 0; tmp[i] == 0; ++i);
		  if (cnt_h == BITS_PER_MP_LIMB - 1)
		    {
		      MPN_COPY (frac, tmp + i, tmpsize - i);
		      fracsize = tmpsize - i;
		    }
		  else
		    {
		      count_trailing_zeros (cnt_l, tmp[i]);

		      /* Now shift the numbers to their optimal position.  */
		      if (i == 0 && BITS_PER_MP_LIMB - 1 - cnt_h > cnt_l)
			{
			  /* We cannot save any memory.	 Just roll the
			     number so that the leading digit is in a
			     separate limb.  */

			  cy = __mpn_lshift (frac, tmp, tmpsize, cnt_h + 1);
			  fracsize = tmpsize + 1;
			  frac[fracsize - 1] = cy;
			}
		      else if (BITS_PER_MP_LIMB - 1 - cnt_h <= cnt_l)
			{
			  (void) __mpn_rshift (frac, tmp + i, tmpsize - i,
					       BITS_PER_MP_LIMB - 1 - cnt_h);
			  fracsize = tmpsize - i;
			}
		      else
			{
			  /* We can only save the memory of the limbs which
			     are zero.	The non-zero parts occupy the same
			     number of limbs.  */

			  (void) __mpn_rshift (frac, tmp + (i - 1),
					       tmpsize - (i - 1),
					       BITS_PER_MP_LIMB - 1 - cnt_h);
			  fracsize = tmpsize - (i - 1);
			}
		    }
		  used_limbs = fracsize - 1;
		}
	    }
	  --explog;
	}
      while (powers != &_fpioconst_pow10[1] && exponent > 0);
      /* All factors but 10^-1 are tested now.	*/
      if (exponent > 0)
	{
	  int cnt_l;

	  cy = __mpn_mul_1 (tmp, frac, fracsize, 10);
	  tmpsize = fracsize;
	  assert (cy == 0 || tmp[tmpsize - 1] < 20);

	  count_trailing_zeros (cnt_l, tmp[0]);
	  if (cnt_l < MIN (4, exponent))
	    {
	      cy = __mpn_lshift (frac, tmp, tmpsize,
				 BITS_PER_MP_LIMB - MIN (4, exponent));
	      if (cy != 0)
		frac[tmpsize++] = cy;
	    }
	  else
	    (void) __mpn_rshift (frac, tmp, tmpsize, MIN (4, exponent));
	  fracsize = tmpsize;
	  exp10 |= 1;
	  assert (frac[fracsize - 1] < 10);
	}
      exponent = exp10;
    }
  else
    {
      /* This is a special case.  We don't need a factor because the
	 numbers are in the range of 0.0 <= fp < 8.0.  We simply
	 shift it to the right place and divide it by 1.0 to get the
	 leading digit.	 (Of course this division is not really made.)	*/
      assert (0 <= exponent && exponent < 3 &&
	      exponent + to_shift < BITS_PER_MP_LIMB);

      /* Now shift the input value to its right place.	*/
      cy = __mpn_lshift (frac, fp_input, fracsize, (exponent + to_shift));
      frac[fracsize++] = cy;
      exponent = 0;
    }

  {
    int width = info->width;
    wchar_t *wbuffer, *wstartp, *wcp;
    int buffer_malloced;
    int chars_needed;
    int expscale;
    int intdig_max, intdig_no = 0;
    int fracdig_min, fracdig_max, fracdig_no = 0;
    int dig_max;
    int significant;
    int ngroups = 0;

    if (_tolower (info->spec) == 'e')
      {
	type = info->spec;
	intdig_max = 1;
	fracdig_min = fracdig_max = info->prec < 0 ? 6 : info->prec;
	chars_needed = 1 + 1 + fracdig_max + 1 + 1 + 4;
	/*	       d   .	 ddd	     e	 +-  ddd  */
	dig_max = INT_MAX;		/* Unlimited.  */
	significant = 1;		/* Does not matter here.  */
      }
    else if (info->spec == 'f')
      {
	type = 'f';
	fracdig_min = fracdig_max = info->prec < 0 ? 6 : info->prec;
	if (expsign == 0)
	  {
	    intdig_max = exponent + 1;
	    /* This can be really big!	*/  /* XXX Maybe malloc if too big? */
	    chars_needed = exponent + 1 + 1 + fracdig_max;
	  }
	else
	  {
	    intdig_max = 1;
	    chars_needed = 1 + 1 + fracdig_max;
	  }
	dig_max = INT_MAX;		/* Unlimited.  */
	significant = 1;		/* Does not matter here.  */
      }
    else
      {
	dig_max = info->prec < 0 ? 6 : (info->prec == 0 ? 1 : info->prec);
	if ((expsign == 0 && exponent >= dig_max)
	    || (expsign != 0 && exponent > 4))
	  {
	    if ('g' - 'G' == 'e' - 'E')
	      type = 'E' + (info->spec - 'G');
	    else
	      type = isupper (info->spec) ? 'E' : 'e';
	    fracdig_max = dig_max - 1;
	    intdig_max = 1;
	    chars_needed = 1 + 1 + fracdig_max + 1 + 1 + 4;
	  }
	else
	  {
	    type = 'f';
	    intdig_max = expsign == 0 ? exponent + 1 : 0;
	    fracdig_max = dig_max - intdig_max;
	    /* We need space for the significant digits and perhaps
	       for leading zeros when < 1.0.  The number of leading
	       zeros can be as many as would be required for
	       exponential notation with a negative two-digit
	       exponent, which is 4.  */
	    chars_needed = dig_max + 1 + 4;
	  }
	fracdig_min = info->alt ? fracdig_max : 0;
	significant = 0;		/* We count significant digits.	 */
      }

    if (grouping)
      {
	/* Guess the number of groups we will make, and thus how
	   many spaces we need for separator characters.  */
	ngroups = __guess_grouping (intdig_max, grouping);
	chars_needed += ngroups;
      }

    /* Allocate buffer for output.  We need two more because while rounding
       it is possible that we need two more characters in front of all the
       other output.  If the amount of memory we have to allocate is too
       large use `malloc' instead of `alloca'.  */
    buffer_malloced = chars_needed > 5000;
    if (buffer_malloced)
      {
	wbuffer = (wchar_t *) malloc ((2 + chars_needed) * sizeof (wchar_t));
	if (wbuffer == NULL)
	  /* Signal an error to the caller.  */
	  return -1;
      }
    else
      wbuffer = (wchar_t *) alloca ((2 + chars_needed) * sizeof (wchar_t));
    wcp = wstartp = wbuffer + 2;	/* Let room for rounding.  */

    /* Do the real work: put digits in allocated buffer.  */
    if (expsign == 0 || type != 'f')
      {
	assert (expsign == 0 || intdig_max == 1);
	while (intdig_no < intdig_max)
	  {
	    ++intdig_no;
	    hack_digit_callee = 1;
	    goto hack_digit;
hack_digit_callee1:
	    *wcp++ = hack_digit_ret;
	  }
	significant = 1;
	if (info->alt
	    || fracdig_min > 0
	    || (fracdig_max > 0 && (fracsize > 1 || frac[0] != 0)))
	  *wcp++ = decimalwc;
      }
    else
      {
	/* |fp| < 1.0 and the selected type is 'f', so put "0."
	   in the buffer.  */
	*wcp++ = L'0';
	--exponent;
	*wcp++ = decimalwc;
      }

    /* Generate the needed number of fractional digits.	 */
    while (fracdig_no < fracdig_min
	   || (fracdig_no < fracdig_max && (fracsize > 1 || frac[0] != 0)))
      {
	++fracdig_no;
	hack_digit_callee = 2;
	goto hack_digit;
hack_digit_callee2:
	*wcp = hack_digit_ret;
	if (*wcp != L'0')
	  significant = 1;
	else if (significant == 0)
	  {
	    ++fracdig_max;
	    if (fracdig_min > 0)
	      ++fracdig_min;
	  }
	++wcp;
      }

    /* Do rounding.  */
    hack_digit_callee = 3;
    goto hack_digit;
hack_digit_callee3:
    digit = hack_digit_ret;
    if (digit > L'4')
      {
	wchar_t *wtp = wcp;

	if (digit == L'5'
	    && ((*(wcp - 1) != decimalwc && (*(wcp - 1) & 1) == 0)
		|| ((*(wcp - 1) == decimalwc && (*(wcp - 2) & 1) == 0))))
	  {
	    /* This is the critical case.	 */
	    if (fracsize == 1 && frac[0] == 0)
	      /* Rest of the number is zero -> round to even.
		 (IEEE 754-1985 4.1 says this is the default rounding.)  */
	      goto do_expo;
	    else if (scalesize == 0)
	      {
		/* Here we have to see whether all limbs are zero since no
		   normalization happened.  */
		size_t lcnt = fracsize;
		while (lcnt >= 1 && frac[lcnt - 1] == 0)
		  --lcnt;
		if (lcnt == 0)
		  /* Rest of the number is zero -> round to even.
		     (IEEE 754-1985 4.1 says this is the default rounding.)  */
		  goto do_expo;
	      }
	  }

	if (fracdig_no > 0)
	  {
	    /* Process fractional digits.  Terminate if not rounded or
	       radix character is reached.  */
	    while (*--wtp != decimalwc && *wtp == L'9')
	      *wtp = '0';
	    if (*wtp != decimalwc)
	      /* Round up.  */
	      (*wtp)++;
	  }

	if (fracdig_no == 0 || *wtp == decimalwc)
	  {
	    /* Round the integer digits.  */
	    if (*(wtp - 1) == decimalwc)
	      --wtp;

	    while (--wtp >= wstartp && *wtp == L'9')
	      *wtp = L'0';

	    if (wtp >= wstartp)
	      /* Round up.  */
	      (*wtp)++;
	    else
	      /* It is more critical.  All digits were 9's.  */
	      {
		if (type != 'f')
		  {
		    *wstartp = '1';
		    exponent += expsign == 0 ? 1 : -1;
		  }
		else if (intdig_no == dig_max)
		  {
		    /* This is the case where for type %g the number fits
		       really in the range for %f output but after rounding
		       the number of digits is too big.	 */
		    *--wstartp = decimalwc;
		    *--wstartp = L'1';

		    if (info->alt || fracdig_no > 0)
		      {
			/* Overwrite the old radix character.  */
			wstartp[intdig_no + 2] = L'0';
			++fracdig_no;
		      }

		    fracdig_no += intdig_no;
		    intdig_no = 1;
		    fracdig_max = intdig_max - intdig_no;
		    ++exponent;
		    /* Now we must print the exponent.	*/
		    type = isupper (info->spec) ? 'E' : 'e';
		  }
		else
		  {
		    /* We can simply add another another digit before the
		       radix.  */
		    *--wstartp = L'1';
		    ++intdig_no;
		  }

		/* While rounding the number of digits can change.
		   If the number now exceeds the limits remove some
		   fractional digits.  */
		if (intdig_no + fracdig_no > dig_max)
		  {
		    wcp -= intdig_no + fracdig_no - dig_max;
		    fracdig_no -= intdig_no + fracdig_no - dig_max;
		  }
	      }
	  }
      }

  do_expo:
    /* Now remove unnecessary '0' at the end of the string.  */
    while (fracdig_no > fracdig_min && *(wcp - 1) == L'0')
      {
	--wcp;
	--fracdig_no;
      }
    /* If we eliminate all fractional digits we perhaps also can remove
       the radix character.  */
    if (fracdig_no == 0 && !info->alt && *(wcp - 1) == decimalwc)
      --wcp;

    if (grouping)
      /* Add in separator characters, overwriting the same buffer.  */
      wcp = group_number (wstartp, wcp, intdig_no, grouping, thousands_sepwc,
			  ngroups);

    /* Write the exponent if it is needed.  */
    if (type != 'f')
      {
	*wcp++ = (wchar_t) type;
	*wcp++ = expsign ? L'-' : L'+';

	/* Find the magnitude of the exponent.	*/
	expscale = 10;
	while (expscale <= exponent)
	  expscale *= 10;

	if (exponent < 10)
	  /* Exponent always has at least two digits.  */
	  *wcp++ = L'0';
	else
	  do
	    {
	      expscale /= 10;
	      *wcp++ = L'0' + (exponent / expscale);
	      exponent %= expscale;
	    }
	  while (expscale > 10);
	*wcp++ = L'0' + exponent;
      }

    /* Compute number of characters which must be filled with the padding
       character.  */
    if (is_neg || info->showsign || info->space)
      --width;
    width -= wcp - wstartp;

    if (!info->left && info->pad != '0' && width > 0)
      PADN (info->pad, width);

    if (is_neg)
      outchar ('-');
    else if (info->showsign)
      outchar ('+');
    else if (info->space)
      outchar (' ');

    if (!info->left && info->pad == '0' && width > 0)
      PADN ('0', width);

    {
      char *buffer = NULL;
      char *cp = NULL;
      char *tmpptr;

      if (! wide)
	{
	  /* Create the single byte string.  */
	  size_t decimal_len;
	  size_t thousands_sep_len;
	  wchar_t *copywc;

	  decimal_len = strlen (decimal);

	  if (thousands_sep == NULL)
	    thousands_sep_len = 0;
	  else
	    thousands_sep_len = strlen (thousands_sep);

	  if (buffer_malloced)
	    {
	      buffer = (char *) malloc (2 + chars_needed + decimal_len
					+ ngroups * thousands_sep_len);
	      if (buffer == NULL)
		/* Signal an error to the caller.  */
		return -1;
	    }
	  else
	    buffer = (char *) alloca (2 + chars_needed + decimal_len
				      + ngroups * thousands_sep_len);

	  /* Now copy the wide character string.  Since the character
	     (except for the decimal point and thousands separator) must
	     be coming from the ASCII range we can esily convert the
	     string without mapping tables.  */
	  for (cp = buffer, copywc = wstartp; copywc < wcp; ++copywc)
	    if (*copywc == decimalwc)
	      cp = (char *) __mempcpy (cp, decimal, decimal_len);
	    else if (*copywc == thousands_sepwc)
	      cp = (char *) __mempcpy (cp, thousands_sep, thousands_sep_len);
	    else
	      *cp++ = (char) *copywc;
	}

      tmpptr = buffer;
      PRINT (tmpptr, wstartp, wide ? wcp - wstartp : cp - tmpptr);

      /* Free the memory if necessary.  */
      if (buffer_malloced)
	{
	  free (buffer);
	  free (wbuffer);
	}
    }

    if (info->left && width > 0)
      PADN (info->pad, width);
  }
  return done;
}

/* Return the number of extra grouping characters that will be inserted
   into a number with INTDIG_MAX integer digits.  */

unsigned int
__guess_grouping (unsigned int intdig_max, const char *grouping)
{
  unsigned int groups;

  /* We treat all negative values like CHAR_MAX.  */

  if (*grouping == CHAR_MAX || *grouping <= 0)
    /* No grouping should be done.  */
    return 0;

  groups = 0;
  while (intdig_max > (unsigned int) *grouping)
    {
      ++groups;
      intdig_max -= *grouping++;

      if (*grouping == CHAR_MAX
#if CHAR_MIN < 0
	  || *grouping < 0
#endif
	  )
	/* No more grouping should be done.  */
	break;
      else if (*grouping == 0)
	{
	  /* Same grouping repeats.  */
	  groups += (intdig_max - 1) / grouping[-1];
	  break;
	}
    }

  return groups;
}

/* Group the INTDIG_NO integer digits of the number in [BUF,BUFEND).
   There is guaranteed enough space past BUFEND to extend it.
   Return the new end of buffer.  */

static wchar_t *
internal_function
group_number (wchar_t *buf, wchar_t *bufend, unsigned int intdig_no,
	      const char *grouping, wchar_t thousands_sep, int ngroups)
{
  wchar_t *p;

  if (ngroups == 0)
    return bufend;

  /* Move the fractional part down.  */
  __wmemmove (buf + intdig_no + ngroups, buf + intdig_no,
	      bufend - (buf + intdig_no));

  p = buf + intdig_no + ngroups - 1;
  do
    {
      unsigned int len = *grouping++;
      do
	*p-- = buf[--intdig_no];
      while (--len > 0);
      *p-- = thousands_sep;

      if (*grouping == CHAR_MAX
#if CHAR_MIN < 0
	  || *grouping < 0
#endif
	  )
	/* No more grouping should be done.  */
	break;
      else if (*grouping == 0)
	/* Same grouping repeats.  */
	--grouping;
    } while (intdig_no > (unsigned int) *grouping);

  /* Copy the remaining ungrouped digits.  */
  do
    *p-- = buf[--intdig_no];
  while (p > buf);

  return bufend + ngroups;
}
