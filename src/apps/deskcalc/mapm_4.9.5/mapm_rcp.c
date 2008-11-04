
/* 
 *  M_APM  -  mapm_rcp.c
 *
 *  Copyright (C) 2000 - 2007   Michael C. Ring
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
 *      $Id: mapm_rcp.c,v 1.7 2007/12/03 01:46:46 mike Exp $
 *
 *      This file contains the fast division and reciprocal functions
 *
 *      $Log: mapm_rcp.c,v $
 *      Revision 1.7  2007/12/03 01:46:46  mike
 *      Update license
 *
 *      Revision 1.6  2003/07/21 20:20:17  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.5  2003/05/01 21:58:40  mike
 *      remove math.h
 *
 *      Revision 1.4  2003/03/31 22:15:49  mike
 *      call generic error handling function
 *
 *      Revision 1.3  2002/11/03 21:32:09  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.2  2000/09/26 16:27:48  mike
 *      add some comments
 *
 *      Revision 1.1  2000/09/26 16:16:00  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
void	m_apm_divide(M_APM rr, int places, M_APM aa, M_APM bb)
{
M_APM   tmp0, tmp1;
int     sn, nexp, dplaces;

sn = aa->m_apm_sign * bb->m_apm_sign;

if (sn == 0)                  /* one number is zero, result is zero */
  {
   if (bb->m_apm_sign == 0)
     {
      M_apm_log_error_msg(M_APM_RETURN, "\'m_apm_divide\', Divide by 0");
     }

   M_set_to_zero(rr);
   return;
  }

/*
 *    Use the original 'Knuth' method for smaller divides. On the
 *    author's system, this was the *approx* break even point before
 *    the reciprocal method used below became faster.
 */

if (places < 250)
  {
   M_apm_sdivide(rr, places, aa, bb);
   return;
  }

/* mimic the decimal place behavior of the original divide */

nexp = aa->m_apm_exponent - bb->m_apm_exponent;

if (nexp > 0)
  dplaces = nexp + places;
else
  dplaces = places;

tmp0 = M_get_stack_var();
tmp1 = M_get_stack_var();

m_apm_reciprocal(tmp0, (dplaces + 8), bb);
m_apm_multiply(tmp1, tmp0, aa);
m_apm_round(rr, dplaces, tmp1);

M_restore_stack(2);
}
/****************************************************************************/
void	m_apm_reciprocal(M_APM rr, int places, M_APM aa)
{
M_APM   last_x, guess, tmpN, tmp1, tmp2;
char    sbuf[32];
int	ii, bflag, dplaces, nexp, tolerance;

if (aa->m_apm_sign == 0)
  {
   M_apm_log_error_msg(M_APM_RETURN, "\'m_apm_reciprocal\', Input = 0");

   M_set_to_zero(rr);
   return;
  }

last_x = M_get_stack_var();
guess  = M_get_stack_var();
tmpN   = M_get_stack_var();
tmp1   = M_get_stack_var();
tmp2   = M_get_stack_var();

m_apm_absolute_value(tmpN, aa);

/* 
    normalize the input number (make the exponent 0) so
    the 'guess' below will not over/under flow on large
    magnitude exponents.
*/

nexp = aa->m_apm_exponent;
tmpN->m_apm_exponent -= nexp;

m_apm_to_string(sbuf, 15, tmpN);
m_apm_set_double(guess, (1.0 / atof(sbuf)));

tolerance = places + 4;
dplaces   = places + 16;
bflag     = FALSE;

m_apm_negate(last_x, MM_Ten);

/*   Use the following iteration to calculate the reciprocal :


         X     =  X  *  [ 2 - N * X ]
          n+1
*/

ii = 0;

while (TRUE)
  {
   m_apm_multiply(tmp1, tmpN, guess);
   m_apm_subtract(tmp2, MM_Two, tmp1);
   m_apm_multiply(tmp1, tmp2, guess);

   if (bflag)
     break;

   m_apm_round(guess, dplaces, tmp1);

   /* force at least 2 iterations so 'last_x' has valid data */

   if (ii != 0)
     {
      m_apm_subtract(tmp2, guess, last_x);

      if (tmp2->m_apm_sign == 0)
        break;

      /* 
       *   if we are within a factor of 4 on the error term,
       *   we will be accurate enough after the *next* iteration
       *   is complete.
       */

      if ((-4 * tmp2->m_apm_exponent) > tolerance)
        bflag = TRUE;
     }

   m_apm_copy(last_x, guess);
   ii++;
  }

m_apm_round(rr, places, tmp1);
rr->m_apm_exponent -= nexp;
rr->m_apm_sign = aa->m_apm_sign;
M_restore_stack(5);
}
/****************************************************************************/
