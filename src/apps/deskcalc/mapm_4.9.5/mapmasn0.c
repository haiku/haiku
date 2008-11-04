
/* 
 *  M_APM  -  mapmasn0.c
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
 *      $Id: mapmasn0.c,v 1.8 2007/12/03 01:49:49 mike Exp $
 *
 *      This file contains the 'ARC' family of functions; ARC-SIN, 
 *	ARC-COS, ARC-TAN when the input arg is very close to 0 (zero).
 *
 *      $Log: mapmasn0.c,v $
 *      Revision 1.8  2007/12/03 01:49:49  mike
 *      Update license
 *
 *      Revision 1.7  2003/06/02 16:51:13  mike
 *      *** empty log message ***
 *
 *      Revision 1.6  2003/06/02 16:49:48  mike
 *      tweak the decimal places
 *
 *      Revision 1.5  2003/06/02 16:47:39  mike
 *      tweak arctan algorithm some more
 *
 *      Revision 1.4  2003/05/31 22:38:07  mike
 *      optimize arctan by using fewer digits as subsequent
 *      terms get smaller
 *
 *      Revision 1.3  2002/11/03 21:36:43  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.2  2000/12/02 20:11:37  mike
 *      add comments
 *
 *      Revision 1.1  2000/12/02 20:08:27  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
/*
        Calculate arcsin using the identity :

                                      x
        arcsin (x) == arctan [ --------------- ]
                                sqrt(1 - x^2)

*/
void	M_arcsin_near_0(M_APM rr, int places, M_APM aa)
{
M_APM   tmp5, tmp6;

tmp5 = M_get_stack_var();
tmp6 = M_get_stack_var();

M_cos_to_sin(tmp5, (places + 8), aa);
m_apm_divide(tmp6, (places + 8), aa, tmp5);
M_arctan_near_0(rr, places, tmp6);

M_restore_stack(2);
}
/****************************************************************************/
/*
        Calculate arccos using the identity :

        arccos (x) == PI / 2 - arcsin (x)

*/
void	M_arccos_near_0(M_APM rr, int places, M_APM aa)
{
M_APM   tmp1, tmp2;

tmp1 = M_get_stack_var();
tmp2 = M_get_stack_var();

M_check_PI_places(places);
M_arcsin_near_0(tmp1, (places + 4), aa);
m_apm_subtract(tmp2, MM_lc_HALF_PI, tmp1);
m_apm_round(rr, places, tmp2);

M_restore_stack(2);
}
/****************************************************************************/
/*
	calculate arctan (x) with the following series:

                              x^3     x^5     x^7     x^9
	arctan (x)  =   x  -  ---  +  ---  -  ---  +  ---  ...
                               3       5       7       9

*/
void	M_arctan_near_0(M_APM rr, int places, M_APM aa)
{
M_APM   tmp0, tmp2, tmpR, tmpS, digit, term;
int	tolerance, dplaces, local_precision;
long    m1;

tmp0  = M_get_stack_var();
tmp2  = M_get_stack_var();
tmpR  = M_get_stack_var();
tmpS  = M_get_stack_var();
term  = M_get_stack_var();
digit = M_get_stack_var();

tolerance = aa->m_apm_exponent - (places + 4);
dplaces   = (places + 8) - aa->m_apm_exponent;

m_apm_copy(term, aa);
m_apm_copy(tmpS, aa);
m_apm_multiply(tmp0, aa, aa);
m_apm_round(tmp2, (dplaces + 8), tmp0);

m1 = 1L;

while (TRUE)
  {
   /*
    *   do the subtraction term
    */

   m_apm_multiply(tmp0, term, tmp2);

   if ((tmp0->m_apm_exponent < tolerance) || (tmp0->m_apm_sign == 0))
     {
      m_apm_round(rr, places, tmpS);
      break;
     }

   local_precision = dplaces + tmp0->m_apm_exponent;

   if (local_precision < 20)
     local_precision = 20;

   m1 += 2;
   m_apm_set_long(digit, m1);
   m_apm_round(term, local_precision, tmp0);
   m_apm_divide(tmp0, local_precision, term, digit);
   m_apm_subtract(tmpR, tmpS, tmp0);

   /*
    *   do the addition term
    */

   m_apm_multiply(tmp0, term, tmp2);

   if ((tmp0->m_apm_exponent < tolerance) || (tmp0->m_apm_sign == 0))
     {
      m_apm_round(rr, places, tmpR);
      break;
     }

   local_precision = dplaces + tmp0->m_apm_exponent;

   if (local_precision < 20)
     local_precision = 20;

   m1 += 2;
   m_apm_set_long(digit, m1);
   m_apm_round(term, local_precision, tmp0);
   m_apm_divide(tmp0, local_precision, term, digit);
   m_apm_add(tmpS, tmpR, tmp0);
  }

M_restore_stack(6);                    /* restore the 6 locals we used here */
}
/****************************************************************************/
