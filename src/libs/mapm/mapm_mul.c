
/* 
 *  M_APM  -  mapm_mul.c
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
 *      $Id: mapm_mul.c,v 1.16 2007/12/03 01:45:27 mike Exp $
 *
 *      This file contains basic multiplication function.
 *
 *      $Log: mapm_mul.c,v $
 *      Revision 1.16  2007/12/03 01:45:27  mike
 *      Update license
 *
 *      Revision 1.15  2004/02/21 04:30:35  mike
 *      minor optimization by using pointers instead of array indexes.
 *
 *      Revision 1.14  2003/07/21 20:18:59  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.13  2003/03/31 22:14:05  mike
 *      call generic error handling function
 *
 *      Revision 1.12  2002/11/03 22:25:36  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.11  2001/07/24 18:24:26  mike
 *      access div/rem lookup table directly
 *      for speed
 *
 *      Revision 1.10  2001/02/11 22:31:39  mike
 *      modify parameters to REALLOC
 *
 *      Revision 1.9  2000/07/09 00:20:03  mike
 *      change break even point again ....
 *
 *      Revision 1.8  2000/07/08 18:51:43  mike
 *      change break even point between this O(n^2)
 *      multiply and the FFT multiply
 *
 *      Revision 1.7  2000/04/14 16:27:45  mike
 *      change the break even point between the 2 multiply
 *      functions since we made the fast one even faster.
 *
 *      Revision 1.6  2000/02/03 22:46:40  mike
 *      use MAPM_* generic memory function
 *
 *      Revision 1.5  1999/09/19 21:10:14  mike
 *      change the break even point between the 2 multiply choices
 *
 *      Revision 1.4  1999/08/09 23:57:17  mike
 *      added more comments
 *
 *      Revision 1.3  1999/08/09 02:38:17  mike
 *      tweak break even point and add comments
 *
 *      Revision 1.2  1999/08/08 18:35:20  mike
 *      add call to fast algorithm if input numbers are large
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

extern void M_fast_multiply(M_APM, M_APM, M_APM);

/****************************************************************************/
void	m_apm_multiply(M_APM r, M_APM a, M_APM b)
{
int	ai, itmp, sign, nexp, ii, jj, indexa, indexb, index0, numdigits;
UCHAR   *cp, *cpr, *cp_div, *cp_rem;
void	*vp;

sign = a->m_apm_sign * b->m_apm_sign;
nexp = a->m_apm_exponent + b->m_apm_exponent;

if (sign == 0)      /* one number is zero, result is zero */
  {
   M_set_to_zero(r);
   return;
  }

numdigits = a->m_apm_datalength + b->m_apm_datalength;
indexa = (a->m_apm_datalength + 1) >> 1;
indexb = (b->m_apm_datalength + 1) >> 1;

/* 
 *	If we are multiplying 2 'big' numbers, use the fast algorithm.
 *
 *	This is a **very** approx break even point between this algorithm
 *      and the FFT multiply. Note that different CPU's, operating systems,
 *      and compiler's may yield a different break even point. This point
 *      (~96 decimal digits) is how the test came out on the author's system.
 */

if (indexa >= 48 && indexb >= 48)
  {
   M_fast_multiply(r, a, b);
   return;
  }

ii = (numdigits + 1) >> 1;     /* required size of result, in bytes */

if (ii > r->m_apm_malloclength)
  {
   if ((vp = MAPM_REALLOC(r->m_apm_data, (ii + 32))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'m_apm_multiply\', Out of memory");
     }
  
   r->m_apm_malloclength = ii + 28;
   r->m_apm_data = (UCHAR *)vp;
  }

M_get_div_rem_addr(&cp_div, &cp_rem);

index0 = indexa + indexb;
cp = r->m_apm_data;
memset(cp, 0, index0);
ii = indexa;

while (TRUE)
  {
   index0--;
   cpr = cp + index0;
   jj  = indexb;
   ai  = (int)a->m_apm_data[--ii];

   while (TRUE)
     {
      itmp = ai * b->m_apm_data[--jj];

      *(cpr-1) += cp_div[itmp];
      *cpr     += cp_rem[itmp];

      if (*cpr >= 100)
        {
         *cpr     -= 100;
         *(cpr-1) += 1;
	}

      cpr--;

      if (*cpr >= 100)
        {
         *cpr     -= 100;
         *(cpr-1) += 1;
	}

      if (jj == 0)
        break;
     }

   if (ii == 0)
     break;
  }

r->m_apm_sign       = sign;
r->m_apm_exponent   = nexp;
r->m_apm_datalength = numdigits;

M_apm_normalize(r);
}
/****************************************************************************/
