
/* 
 *  M_APM  -  mapmsqrt.c
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
 *      $Id: mapmsqrt.c,v 1.19 2007/12/03 01:57:31 mike Exp $
 *
 *      This file contains the SQRT function.
 *
 *      $Log: mapmsqrt.c,v $
 *      Revision 1.19  2007/12/03 01:57:31  mike
 *      Update license
 *
 *      Revision 1.18  2003/07/21 20:39:00  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.17  2003/05/07 16:36:04  mike
 *      simplify 'nexp' logic
 *
 *      Revision 1.16  2003/03/31 21:50:14  mike
 *      call generic error handling function
 *
 *      Revision 1.15  2003/03/11 21:29:00  mike
 *      round an intermediate result for faster runtime.
 *
 *      Revision 1.14  2002/11/03 22:00:46  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.13  2001/07/10 22:50:31  mike
 *      minor optimization
 *
 *      Revision 1.12  2000/09/26 18:32:04  mike
 *      use new algorithm which only uses multiply and subtract
 *      (avoids the slower version which used division)
 *
 *      Revision 1.11  2000/07/11 17:56:22  mike
 *      make better estimate for initial precision
 *
 *      Revision 1.10  1999/07/21 02:48:45  mike
 *      added some comments
 *
 *      Revision 1.9  1999/07/19 00:25:44  mike
 *      adjust local precision again
 *
 *      Revision 1.8  1999/07/19 00:09:41  mike
 *      adjust local precision during loop
 *
 *      Revision 1.7  1999/07/18 22:57:08  mike
 *      change to dynamically changing local precision and
 *      change tolerance checks using integers
 *
 *      Revision 1.6  1999/06/19 21:18:00  mike
 *      changed local static variables to MAPM stack variables
 *
 *      Revision 1.5  1999/05/31 01:40:39  mike
 *      minor update to normalizing the exponent
 *
 *      Revision 1.4  1999/05/31 01:21:41  mike
 *      optimize for large exponents
 *
 *      Revision 1.3  1999/05/12 20:59:35  mike
 *      use a better 'guess' function
 *
 *      Revision 1.2  1999/05/10 21:15:26  mike
 *      added some comments
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
void	m_apm_sqrt(M_APM rr, int places, M_APM aa)
{
M_APM   last_x, guess, tmpN, tmp7, tmp8, tmp9;
int	ii, bflag, nexp, tolerance, dplaces;

if (aa->m_apm_sign <= 0)
  {
   if (aa->m_apm_sign == -1)
     {
      M_apm_log_error_msg(M_APM_RETURN, "\'m_apm_sqrt\', Negative argument");
     }

   M_set_to_zero(rr);
   return;
  }

last_x = M_get_stack_var();
guess  = M_get_stack_var();
tmpN   = M_get_stack_var();
tmp7   = M_get_stack_var();
tmp8   = M_get_stack_var();
tmp9   = M_get_stack_var();

m_apm_copy(tmpN, aa);

/* 
    normalize the input number (make the exponent near 0) so
    the 'guess' function will not over/under flow on large
    magnitude exponents.
*/

nexp = aa->m_apm_exponent / 2;
tmpN->m_apm_exponent -= 2 * nexp;

M_get_sqrt_guess(guess, tmpN);    /* actually gets 1/sqrt guess */

tolerance = places + 4;
dplaces   = places + 16;
bflag     = FALSE;

m_apm_negate(last_x, MM_Ten);

/*   Use the following iteration to calculate 1 / sqrt(N) :

         X    =  0.5 * X * [ 3 - N * X^2 ]
          n+1                    
*/

ii = 0;

while (TRUE)
  {
   m_apm_multiply(tmp9, tmpN, guess);
   m_apm_multiply(tmp8, tmp9, guess);
   m_apm_round(tmp7, dplaces, tmp8);
   m_apm_subtract(tmp9, MM_Three, tmp7);
   m_apm_multiply(tmp8, tmp9, guess);
   m_apm_multiply(tmp9, tmp8, MM_0_5);

   if (bflag)
     break;

   m_apm_round(guess, dplaces, tmp9);

   /* force at least 2 iterations so 'last_x' has valid data */

   if (ii != 0)
     {
      m_apm_subtract(tmp7, guess, last_x);

      if (tmp7->m_apm_sign == 0)
        break;

      /* 
       *   if we are within a factor of 4 on the error term,
       *   we will be accurate enough after the *next* iteration
       *   is complete.  (note that the sign of the exponent on 
       *   the error term will be a negative number).
       */

      if ((-4 * tmp7->m_apm_exponent) > tolerance)
        bflag = TRUE;
     }

   m_apm_copy(last_x, guess);
   ii++;
  }

/*
 *    multiply by the starting number to get the final
 *    sqrt and then adjust the exponent since we found
 *    the sqrt of the normalized number.
 */

m_apm_multiply(tmp8, tmp9, tmpN);
m_apm_round(rr, places, tmp8);
rr->m_apm_exponent += nexp;

M_restore_stack(6);
}
/****************************************************************************/
