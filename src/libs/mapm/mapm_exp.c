
/* 
 *  M_APM  -  mapm_exp.c
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
 *      $Id: mapm_exp.c,v 1.37 2007/12/03 01:36:00 mike Exp $
 *
 *      This file contains the EXP function.
 *
 *      $Log: mapm_exp.c,v $
 *      Revision 1.37  2007/12/03 01:36:00  mike
 *      Update license
 *
 *      Revision 1.36  2004/06/02 00:29:03  mike
 *      simplify logic in compute_nn
 *
 *      Revision 1.35  2004/05/29 18:29:44  mike
 *      move exp nn calculation into its own function
 *
 *      Revision 1.34  2004/05/21 20:41:01  mike
 *      fix potential buffer overflow
 *
 *      Revision 1.33  2004/02/18 02:46:45  mike
 *      fix comment
 *
 *      Revision 1.32  2004/02/18 02:41:35  mike
 *      check to make sure 'nn' does not overflow
 *
 *      Revision 1.31  2004/01/01 00:06:38  mike
 *      make a comment more clear
 *
 *      Revision 1.30  2003/12/31 21:44:57  mike
 *      simplify logic in _exp
 *
 *      Revision 1.29  2003/12/28 00:03:27  mike
 *      dont allow 'tmp7' to get too small prior to divide by 256
 *
 *      Revision 1.28  2003/12/27 22:53:04  mike
 *      change 1024 to 512, if input is already small, call
 *      raw_exp directly and return
 *
 *      Revision 1.27  2003/03/30 21:16:37  mike
 *      use global version of log(2) instead of local copy
 *
 *      Revision 1.26  2002/11/03 22:30:18  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.25  2001/08/06 22:07:00  mike
 *      round the 'nn' calculation to the nearest int
 *
 *      Revision 1.24  2001/08/05 21:58:59  mike
 *      make 1 / log(2) constant shorter
 *
 *      Revision 1.23  2001/07/16 19:10:23  mike
 *      add function M_free_all_exp
 *
 *      Revision 1.22  2001/07/10 21:55:36  mike
 *      optimize raw_exp by using fewer digits as the
 *      subsequent terms get smaller
 *
 *      Revision 1.21  2001/02/06 21:20:31  mike
 *      optimize 'nn' calculation (for the future)
 *
 *      Revision 1.20  2001/02/05 21:55:12  mike
 *      minor optimization, use a multiply instead
 *      of a divide
 *
 *      Revision 1.19  2000/08/22 21:34:41  mike
 *      increase local precision
 *
 *      Revision 1.18  2000/05/18 22:05:22  mike
 *      move _pow to a separate file
 *
 *      Revision 1.17  2000/05/04 23:21:01  mike
 *      use global var 256R
 *
 *      Revision 1.16  2000/03/30 21:33:30  mike
 *      change termination of raw_exp to use ints, not MAPM numbers
 *
 *      Revision 1.15  2000/02/05 22:59:46  mike
 *      adjust decimal places on calculation
 *
 *      Revision 1.14  2000/02/04 20:45:21  mike
 *      re-compute log(2) on the fly if we need a more precise value
 *
 *      Revision 1.13  2000/02/04 19:35:14  mike
 *      use just an approx log(2) for the integer divide
 *
 *      Revision 1.12  2000/02/04 16:47:32  mike
 *      use the real algorithm for EXP
 *
 *      Revision 1.11  1999/09/18 01:27:40  mike
 *      if X is 0 on the pow function, return 0 right away
 *
 *      Revision 1.10  1999/06/19 20:54:07  mike
 *      changed local static MAPM to stack variables
 *
 *      Revision 1.9  1999/06/01 22:37:44  mike
 *      adjust decimal places passed to raw function
 *
 *      Revision 1.8  1999/06/01 01:44:03  mike
 *      change threshold from 1000 to 100 for 65536 divisor
 *
 *      Revision 1.7  1999/06/01 01:03:31  mike
 *      vary 'q' instead of checking input against +/- 10 and +/- 40
 *
 *      Revision 1.6  1999/05/15 01:54:27  mike
 *      add check for number of decimal places
 *
 *      Revision 1.5  1999/05/15 01:09:49  mike
 *      minor tweak to POW decimal places
 *
 *      Revision 1.4  1999/05/13 00:14:00  mike
 *      added more comments
 *
 *      Revision 1.3  1999/05/12 23:39:05  mike
 *      change #places passed to sub functions
 *
 *      Revision 1.2  1999/05/10 21:35:13  mike
 *      added some comments
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

static  M_APM  MM_exp_log2R;
static  M_APM  MM_exp_512R;
static	int    MM_firsttime1 = TRUE;

/****************************************************************************/
void	M_free_all_exp()
{
if (MM_firsttime1 == FALSE)
  {
   m_apm_free(MM_exp_log2R);
   m_apm_free(MM_exp_512R);

   MM_firsttime1 = TRUE;
  }
}
/****************************************************************************/
void	m_apm_exp(M_APM r, int places, M_APM x)
{
M_APM   tmp7, tmp8, tmp9;
int	dplaces, nn, ii;

if (MM_firsttime1)
  {
   MM_firsttime1 = FALSE;

   MM_exp_log2R = m_apm_init();
   MM_exp_512R  = m_apm_init();

   m_apm_set_string(MM_exp_log2R, "1.44269504089");   /* ~ 1 / log(2) */
   m_apm_set_string(MM_exp_512R,  "1.953125E-3");     /*   1 / 512    */
  }

tmp7 = M_get_stack_var();
tmp8 = M_get_stack_var();
tmp9 = M_get_stack_var();

if (x->m_apm_sign == 0)		/* if input == 0, return '1' */
  {
   m_apm_copy(r, MM_One);
   M_restore_stack(3);
   return;
  }

if (x->m_apm_exponent <= -3)  /* already small enough so call _raw directly */
  {
   M_raw_exp(tmp9, (places + 6), x);
   m_apm_round(r, places, tmp9);
   M_restore_stack(3);
   return;
  }

/*
    From David H. Bailey's MPFUN Fortran package :

    exp (t) =  (1 + r + r^2 / 2! + r^3 / 3! + r^4 / 4! ...) ^ q * 2 ^ n

    where q = 256, r = t' / q, t' = t - n Log(2) and where n is chosen so
    that -0.5 Log(2) < t' <= 0.5 Log(2).  Reducing t mod Log(2) and
    dividing by 256 insures that -0.001 < r <= 0.001, which accelerates
    convergence in the above series.

    I use q = 512 and also limit how small 'r' can become. The 'r' used
    here is limited in magnitude from 1.95E-4 < |r| < 1.35E-3. Forcing
    'r' into a narrow range keeps the algorithm 'well behaved'.

    ( the range is [0.1 / 512] to [log(2) / 512] )
*/

if (M_exp_compute_nn(&nn, tmp7, x) != 0)
  {
   M_apm_log_error_msg(M_APM_RETURN, 
      "\'m_apm_exp\', Input too large, Overflow");

   M_set_to_zero(r);
   M_restore_stack(3);
   return;
  }

dplaces = places + 8;

/* check to make sure our log(2) is accurate enough */

M_check_log_places(dplaces);

m_apm_multiply(tmp8, tmp7, MM_lc_log2);
m_apm_subtract(tmp7, x, tmp8);

/*
 *     guarantee that |tmp7| is between 0.1 and 0.9999999....
 *     (in practice, the upper limit only reaches log(2), 0.693... )
 */

while (TRUE)
  {
   if (tmp7->m_apm_sign != 0)
     {
      if (tmp7->m_apm_exponent == 0)
        break;
     }
     
   if (tmp7->m_apm_sign >= 0)
     {
      nn++;
      m_apm_subtract(tmp8, tmp7, MM_lc_log2);
      m_apm_copy(tmp7, tmp8);
     }
   else
     {
      nn--;
      m_apm_add(tmp8, tmp7, MM_lc_log2);
      m_apm_copy(tmp7, tmp8);
     }
  }

m_apm_multiply(tmp9, tmp7, MM_exp_512R);

/* perform the series expansion ... */

M_raw_exp(tmp8, dplaces, tmp9);

/*
 *   raise result to the 512 power
 *
 *   note : x ^ 512  =  (((x ^ 2) ^ 2) ^ 2) ... 9 times
 */

ii = 9;

while (TRUE)
  {
   m_apm_multiply(tmp9, tmp8, tmp8);
   m_apm_round(tmp8, dplaces, tmp9);

   if (--ii == 0)
     break;
  }

/* now compute 2 ^ N */

m_apm_integer_pow(tmp7, dplaces, MM_Two, nn);

m_apm_multiply(tmp9, tmp7, tmp8);
m_apm_round(r, places, tmp9);

M_restore_stack(3);                    /* restore the 3 locals we used here */
}
/****************************************************************************/
/*
	compute  int *n  = round_to_nearest_int(a / log(2))
	         M_APM b = MAPM version of *n

        returns      0: OK
		 -1, 1: failure
*/
int	M_exp_compute_nn(int *n, M_APM b, M_APM a)
{
M_APM	tmp0, tmp1;
void	*vp;
char    *cp, sbuf[48];
int	kk;

*n   = 0;
vp   = NULL;
cp   = sbuf;
tmp0 = M_get_stack_var();
tmp1 = M_get_stack_var();

/* find 'n' and convert it to a normal C int            */
/* we just need an approx 1/log(2) for this calculation */

m_apm_multiply(tmp1, a, MM_exp_log2R);

/* round to the nearest int */

if (tmp1->m_apm_sign >= 0)
  {
   m_apm_add(tmp0, tmp1, MM_0_5);
   m_apm_floor(tmp1, tmp0);
  }
else
  {
   m_apm_subtract(tmp0, tmp1, MM_0_5);
   m_apm_ceil(tmp1, tmp0);
  }

kk = tmp1->m_apm_exponent;
if (kk >= 42)
  {
   if ((vp = (void *)MAPM_MALLOC((kk + 16) * sizeof(char))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'M_exp_compute_nn\', Out of memory");
     }

   cp = (char *)vp;
  }

m_apm_to_integer_string(cp, tmp1);
*n = atoi(cp);

m_apm_set_long(b, (long)(*n));

kk = m_apm_compare(b, tmp1);

if (vp != NULL)
  MAPM_FREE(vp);

M_restore_stack(2);
return(kk);
}
/****************************************************************************/
/*
	calculate the exponential function using the following series :

                              x^2     x^3     x^4     x^5
	exp(x) == 1  +  x  +  ---  +  ---  +  ---  +  ---  ...
                               2!      3!      4!      5!

*/
void	M_raw_exp(M_APM rr, int places, M_APM xx)
{
M_APM   tmp0, digit, term;
int	tolerance,  local_precision, prev_exp;
long    m1;

tmp0  = M_get_stack_var();
term  = M_get_stack_var();
digit = M_get_stack_var();

local_precision = places + 8;
tolerance       = -(places + 4);
prev_exp        = 0;

m_apm_add(rr, MM_One, xx);
m_apm_copy(term, xx);

m1 = 2L;

while (TRUE)
  {
   m_apm_set_long(digit, m1);
   m_apm_multiply(tmp0, term, xx);
   m_apm_divide(term, local_precision, tmp0, digit);
   m_apm_add(tmp0, rr, term);
   m_apm_copy(rr, tmp0);

   if ((term->m_apm_exponent < tolerance) || (term->m_apm_sign == 0))
     break;

   if (m1 != 2L)
     {
      local_precision = local_precision + term->m_apm_exponent - prev_exp;

      if (local_precision < 20)
        local_precision = 20;
     }

   prev_exp = term->m_apm_exponent;
   m1++;
  }

M_restore_stack(3);                    /* restore the 3 locals we used here */
}
/****************************************************************************/
