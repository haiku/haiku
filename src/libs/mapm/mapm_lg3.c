
/* 
 *  M_APM  -  mapm_lg3.c
 *
 *  Copyright (C) 2003 - 2007   Michael C. Ring
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
 *      $Id: mapm_lg3.c,v 1.7 2007/12/03 01:42:59 mike Exp $
 *
 *      This file contains the function to compute log(2), log(10),
 *	and 1/log(10) to the desired precision using an AGM algorithm.
 *
 *      $Log: mapm_lg3.c,v $
 *      Revision 1.7  2007/12/03 01:42:59  mike
 *      Update license
 *
 *      Revision 1.6  2003/12/09 01:25:06  mike
 *      actually compute the first term of the AGM iteration instead
 *      of assuming the inputs a=1 and b=10^-N.
 *
 *      Revision 1.5  2003/12/04 03:19:16  mike
 *      rearrange logic in AGM to be more straight-forward
 *
 *      Revision 1.4  2003/05/01 22:04:37  mike
 *      rearrange some code
 *
 *      Revision 1.3  2003/05/01 21:58:31  mike
 *      remove math.h
 *
 *      Revision 1.2  2003/03/30 22:14:58  mike
 *      add comments
 *
 *      Revision 1.1  2003/03/30 21:18:04  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/*
 *  using the 'R' function (defined below) for 'N' decimal places :
 *
 *
 *                          -N             -N
 *  log(2)  =  R(1, 0.5 * 10  )  -  R(1, 10  ) 
 *
 *
 *                          -N             -N
 *  log(10) =  R(1, 0.1 * 10  )  -  R(1, 10  ) 
 *
 *
 *  In general:
 *
 *                    -N                -N
 *  log(x)  =  R(1, 10  / x)  -  R(1, 10  ) 
 *
 *
 *  I found this on a web site which went into considerable detail
 *  on the history of log(2). This formula is algebraically identical
 *  to the formula specified in J. Borwein and P. Borwein's book 
 *  "PI and the AGM". (reference algorithm 7.2)
 */

/****************************************************************************/
/*
 *	check if our local copy of log(2) & log(10) is precise
 *      enough for our purpose. if not, calculate them so it's
 *	as precise as desired, accurate to at least 'places'.
 */
void	M_check_log_places(int places)
{
M_APM   tmp6, tmp7, tmp8, tmp9;
int     dplaces;

dplaces = places + 4;

if (dplaces > MM_lc_log_digits)
  {
   MM_lc_log_digits = dplaces + 4;
   
   tmp6 = M_get_stack_var();
   tmp7 = M_get_stack_var();
   tmp8 = M_get_stack_var();
   tmp9 = M_get_stack_var();
   
   dplaces += 6 + (int)log10((double)places);
   
   m_apm_copy(tmp7, MM_One);
   tmp7->m_apm_exponent = -places;
   
   M_log_AGM_R_func(tmp8, dplaces, MM_One, tmp7);
   
   m_apm_multiply(tmp6, tmp7, MM_0_5);
   
   M_log_AGM_R_func(tmp9, dplaces, MM_One, tmp6);
   
   m_apm_subtract(MM_lc_log2, tmp9, tmp8);               /* log(2) */

   tmp7->m_apm_exponent -= 1;                            /* divide by 10 */

   M_log_AGM_R_func(tmp9, dplaces, MM_One, tmp7);

   m_apm_subtract(MM_lc_log10, tmp9, tmp8);              /* log(10) */
   m_apm_reciprocal(MM_lc_log10R, dplaces, MM_lc_log10); /* 1 / log(10) */

   M_restore_stack(4);
  }
}
/****************************************************************************/

/*
 *	define a notation for a function 'R' :
 *
 *
 *
 *                                    1
 *      R (a0, b0)  =  ------------------------------
 *
 *                          ----
 *                           \ 
 *                            \     n-1      2    2
 *                      1  -   |   2    *  (a  - b )
 *                            /              n    n
 *                           /
 *                          ----
 *                         n >= 0
 *
 *
 *      where a, b are the classic AGM iteration :
 *
 *     
 *      a    =  0.5 * (a  + b )
 *       n+1            n    n
 *
 *
 *      b    =  sqrt(a  * b )
 *       n+1          n    n
 *
 *
 *
 *      define a variable 'c' for more efficient computation :
 *
 *                                      2     2     2
 *      c    =  0.5 * (a  - b )    ,   c  =  a  -  b
 *       n+1            n    n          n     n     n
 *
 */

/****************************************************************************/
void	M_log_AGM_R_func(M_APM rr, int places, M_APM aa, M_APM bb)
{
M_APM   tmp1, tmp2, tmp3, tmp4, tmpC2, sum, pow_2, tmpA0, tmpB0;
int	tolerance, dplaces;

tmpA0 = M_get_stack_var();
tmpB0 = M_get_stack_var();
tmpC2 = M_get_stack_var();
tmp1  = M_get_stack_var();
tmp2  = M_get_stack_var();
tmp3  = M_get_stack_var();
tmp4  = M_get_stack_var();
sum   = M_get_stack_var();
pow_2 = M_get_stack_var();

tolerance = places + 8;
dplaces   = places + 16;

m_apm_copy(tmpA0, aa);
m_apm_copy(tmpB0, bb);
m_apm_copy(pow_2, MM_0_5);

m_apm_multiply(tmp1, aa, aa);		    /* 0.5 * [ a ^ 2 - b ^ 2 ] */
m_apm_multiply(tmp2, bb, bb);
m_apm_subtract(tmp3, tmp1, tmp2);
m_apm_multiply(sum, MM_0_5, tmp3);

while (TRUE)
  {
   m_apm_subtract(tmp1, tmpA0, tmpB0);      /* C n+1 = 0.5 * [ An - Bn ] */
   m_apm_multiply(tmp4, MM_0_5, tmp1);      /* C n+1 */
   m_apm_multiply(tmpC2, tmp4, tmp4);       /* C n+1 ^ 2 */

   /* do the AGM */

   m_apm_add(tmp1, tmpA0, tmpB0);
   m_apm_multiply(tmp3, MM_0_5, tmp1);

   m_apm_multiply(tmp2, tmpA0, tmpB0);
   m_apm_sqrt(tmpB0, dplaces, tmp2);

   m_apm_round(tmpA0, dplaces, tmp3);

   /* end AGM */

   m_apm_multiply(tmp2, MM_Two, pow_2);
   m_apm_copy(pow_2, tmp2);

   m_apm_multiply(tmp1, tmpC2, pow_2);
   m_apm_add(tmp3, sum, tmp1);

   if ((tmp1->m_apm_sign == 0) || 
      ((-2 * tmp1->m_apm_exponent) > tolerance))
     break;

   m_apm_round(sum, dplaces, tmp3);
  }

m_apm_subtract(tmp4, MM_One, tmp3);
m_apm_reciprocal(rr, places, tmp4);

M_restore_stack(9);
}
/****************************************************************************/
