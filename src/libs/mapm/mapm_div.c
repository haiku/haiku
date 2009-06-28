
/* 
 *  M_APM  -  mapm_div.c
 *
 *  Copyright (C) 1999 - 2007   Michael C. Ring
 *
 *  Permission to use, copy, and distribute this software and its
 *  documentation for any purpose with or without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and
 *  that both that copyright notice and this permission notice appear
 *  in supporting documentation.
 *
 *  Permission to modify the software is granted. Permission to distribute
 *  the modified code is granted. Modifications are to be distributed by
 *  using the file 'license.txt' as a template to modify the file header.
 *  'license.txt' is available in the official MAPM distribution.
 *
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      $Id: mapm_div.c,v 1.12 2007/12/03 01:35:18 mike Exp $
 *
 *      This file contains the basic division functions 
 *
 *      $Log: mapm_div.c,v $
 *      Revision 1.12  2007/12/03 01:35:18  mike
 *      Update license
 *
 *      Revision 1.11  2003/07/21 20:07:13  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.10  2003/03/31 22:09:18  mike
 *      call generic error handling function
 *
 *      Revision 1.9  2002/11/03 22:06:50  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.8  2001/07/16 19:03:22  mike
 *      add function M_free_all_div
 *
 *      Revision 1.7  2001/02/11 22:30:42  mike
 *      modify parameters to REALLOC
 *
 *      Revision 1.6  2000/09/23 19:07:17  mike
 *      change _divide to M_apm_sdivide function name
 *
 *      Revision 1.5  2000/04/11 18:38:55  mike
 *      use new algorithm to determine q-hat. uses more digits of
 *      the numerator and denominator.
 *
 *      Revision 1.4  2000/02/03 22:45:08  mike
 *      use MAPM_* generic memory function
 *
 *      Revision 1.3  1999/06/23 01:10:49  mike
 *      use predefined constant for '15'
 *
 *      Revision 1.2  1999/06/23 00:55:09  mike
 *      change mult factor to 15
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

static	M_APM	M_div_worka;
static	M_APM	M_div_workb;
static	M_APM	M_div_tmp7;
static	M_APM	M_div_tmp8;
static	M_APM	M_div_tmp9;

static	int	M_div_firsttime = TRUE;

/****************************************************************************/
void	M_free_all_div()
{
if (M_div_firsttime == FALSE)
  {
   m_apm_free(M_div_worka);
   m_apm_free(M_div_workb);
   m_apm_free(M_div_tmp7);
   m_apm_free(M_div_tmp8);
   m_apm_free(M_div_tmp9);

   M_div_firsttime = TRUE;
  }
}
/****************************************************************************/
void	m_apm_integer_div_rem(M_APM qq, M_APM rr, M_APM aa, M_APM bb)
{
m_apm_integer_divide(qq, aa, bb);
m_apm_multiply(M_div_tmp7, qq, bb);
m_apm_subtract(rr, aa, M_div_tmp7);
}
/****************************************************************************/
void	m_apm_integer_divide(M_APM rr, M_APM aa, M_APM bb)
{
/*
 *    we must use this divide function since the 
 *    faster divide function using the reciprocal
 *    will round the result (possibly changing 
 *    nnm.999999...  -->  nn(m+1).0000 which would 
 *    invalidate the 'integer_divide' goal).
 */

M_apm_sdivide(rr, 4, aa, bb);

if (rr->m_apm_exponent <= 0)        /* result is 0 */
  {
   M_set_to_zero(rr);
  }
else
  {
   if (rr->m_apm_datalength > rr->m_apm_exponent)
     {
      rr->m_apm_datalength = rr->m_apm_exponent;
      M_apm_normalize(rr);
     }
  }
}
/****************************************************************************/
void	M_apm_sdivide(M_APM r, int places, M_APM a, M_APM b)
{
int	j, k, m, b0, sign, nexp, indexr, icompare, iterations;
long    trial_numer;
void	*vp;

if (M_div_firsttime)
  {
   M_div_firsttime = FALSE;

   M_div_worka = m_apm_init();
   M_div_workb = m_apm_init();
   M_div_tmp7  = m_apm_init();
   M_div_tmp8  = m_apm_init();
   M_div_tmp9  = m_apm_init();
  }

sign = a->m_apm_sign * b->m_apm_sign;

if (sign == 0)      /* one number is zero, result is zero */
  {
   if (b->m_apm_sign == 0)
     {
      M_apm_log_error_msg(M_APM_RETURN, "\'M_apm_sdivide\', Divide by 0");
     }

   M_set_to_zero(r);
   return;
  }

/*
 *  Knuth step D1. Since base = 100, base / 2 = 50.
 *  (also make the working copies positive)
 */

if (b->m_apm_data[0] >= 50)
  {
   m_apm_absolute_value(M_div_worka, a);
   m_apm_absolute_value(M_div_workb, b);
  }
else       /* 'normal' step D1 */
  {
   k = 100 / (b->m_apm_data[0] + 1);
   m_apm_set_long(M_div_tmp9, (long)k);

   m_apm_multiply(M_div_worka, M_div_tmp9, a);
   m_apm_multiply(M_div_workb, M_div_tmp9, b);

   M_div_worka->m_apm_sign = 1;
   M_div_workb->m_apm_sign = 1;
  }

/* setup trial denominator for step D3 */

b0 = 100 * (int)M_div_workb->m_apm_data[0];

if (M_div_workb->m_apm_datalength >= 3)
  b0 += M_div_workb->m_apm_data[1];

nexp = M_div_worka->m_apm_exponent - M_div_workb->m_apm_exponent;

if (nexp > 0)
  iterations = nexp + places + 1;
else
  iterations = places + 1;

k = (iterations + 1) >> 1;     /* required size of result, in bytes */

if (k > r->m_apm_malloclength)
  {
   if ((vp = MAPM_REALLOC(r->m_apm_data, (k + 32))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'M_apm_sdivide\', Out of memory");
     }
  
   r->m_apm_malloclength = k + 28;
   r->m_apm_data = (UCHAR *)vp;
  }

/* clear the exponent in the working copies */

M_div_worka->m_apm_exponent = 0;
M_div_workb->m_apm_exponent = 0;

/* if numbers are equal, ratio == 1.00000... */

if ((icompare = m_apm_compare(M_div_worka, M_div_workb)) == 0)
  {
   iterations = 1;
   r->m_apm_data[0] = 10;
   nexp++;
  }
else			           /* ratio not 1, do the real division */
  {
   if (icompare == 1)                        /* numerator > denominator */
     {
      nexp++;                           /* to adjust the final exponent */
      M_div_worka->m_apm_exponent += 1;     /* multiply numerator by 10 */
     }
   else                                      /* numerator < denominator */
     {
      M_div_worka->m_apm_exponent += 2;    /* multiply numerator by 100 */
     }

   indexr = 0;
   m      = 0;

   while (TRUE)
     {
      /*
       *  Knuth step D3. Only use the 3rd -> 6th digits if the number
       *  actually has that many digits.
       */

      trial_numer = 10000L * (long)M_div_worka->m_apm_data[0];
      
      if (M_div_worka->m_apm_datalength >= 5)
        {
         trial_numer += 100 * M_div_worka->m_apm_data[1]
                            + M_div_worka->m_apm_data[2];
	}
      else
        {
         if (M_div_worka->m_apm_datalength >= 3)
           trial_numer += 100 * M_div_worka->m_apm_data[1];
        }

      j = (int)(trial_numer / b0);

      /* 
       *    Since the library 'normalizes' all the results, we need
       *    to look at the exponent of the number to decide if we 
       *    have a lead in 0n or 00.
       */

      if ((k = 2 - M_div_worka->m_apm_exponent) > 0)
        {
	 while (TRUE)
	   {
	    j /= 10;
	    if (--k == 0)
	      break;
	   }
	}

      if (j == 100)     /* qhat == base ??      */
        j = 99;         /* if so, decrease by 1 */

      m_apm_set_long(M_div_tmp8, (long)j);
      m_apm_multiply(M_div_tmp7, M_div_tmp8, M_div_workb);

      /*
       *    Compare our q-hat (j) against the desired number.
       *    j is either correct, 1 too large, or 2 too large
       *    per Theorem B on pg 272 of Art of Compter Programming,
       *    Volume 2, 3rd Edition.
       *    
       *    The above statement is only true if using the 2 leading
       *    digits of the numerator and the leading digit of the 
       *    denominator. Since we are using the (3) leading digits
       *    of the numerator and the (2) leading digits of the 
       *    denominator, we eliminate the case where our q-hat is 
       *    2 too large, (and q-hat being 1 too large is quite remote).
       */

      if (m_apm_compare(M_div_tmp7, M_div_worka) == 1)
        {
	 j--;
         m_apm_subtract(M_div_tmp8, M_div_tmp7, M_div_workb);
         m_apm_copy(M_div_tmp7, M_div_tmp8);
	}

      /* 
       *  Since we know q-hat is correct, step D6 is unnecessary.
       *
       *  Store q-hat, step D5. Since D6 is unnecessary, we can 
       *  do D5 before D4 and decide if we are done.
       */

      r->m_apm_data[indexr++] = (UCHAR)j;    /* j == 'qhat' */
      m += 2;

      if (m >= iterations)
        break;

      /* step D4 */

      m_apm_subtract(M_div_tmp9, M_div_worka, M_div_tmp7);

      /*
       *  if the subtraction yields zero, the division is exact
       *  and we are done early.
       */

      if (M_div_tmp9->m_apm_sign == 0)
        {
	 iterations = m;
	 break;
	}

      /* multiply by 100 and re-save */
      M_div_tmp9->m_apm_exponent += 2;
      m_apm_copy(M_div_worka, M_div_tmp9);
     }
  }

r->m_apm_sign       = sign;
r->m_apm_exponent   = nexp;
r->m_apm_datalength = iterations;

M_apm_normalize(r);
}
/****************************************************************************/
