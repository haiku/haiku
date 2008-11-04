
/* 
 *  M_APM  -  mapmfact.c
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
 *      $Id: mapmfact.c,v 1.11 2007/12/03 01:51:50 mike Exp $
 *
 *      This file contains the FACTORIAL function.
 *
 *      $Log: mapmfact.c,v $
 *      Revision 1.11  2007/12/03 01:51:50  mike
 *      Update license
 *
 *      Revision 1.10  2003/07/21 21:05:31  mike
 *      facilitate 'gcov' code coverage tool by making an array smaller
 *
 *      Revision 1.9  2002/11/03 21:27:28  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.8  2000/06/14 20:36:16  mike
 *      increase size of DOS array
 *
 *      Revision 1.7  2000/05/29 13:15:59  mike
 *      minor tweaks, fixed comment
 *
 *      Revision 1.6  2000/05/26 16:39:03  mike
 *      minor update to comments and code
 *
 *      Revision 1.5  2000/05/25 23:12:53  mike
 *      change 'nd' calculation
 *
 *      Revision 1.4  2000/05/25 22:17:45  mike
 *      implement new algorithm for speed. approx 5 - 10X
 *      faster on my PC when N! is large (> 6000)
 *
 *      Revision 1.3  1999/06/19 21:25:21  mike
 *      changed local static variables to MAPM stack variables
 *
 *      Revision 1.2  1999/05/23 18:21:12  mike
 *      minor variable name tweaks
 *
 *      Revision 1.1  1999/05/15 21:06:11  mike
 *      Initial revision
 */

/*
 *      Brief explanation of the factorial algorithm.
 *      ----------------------------------------------
 *
 *      The old algorithm simply multiplied N * (N-1) * (N-2) etc, until 
 *	the number counted down to '2'. So one term of the multiplication 
 *	kept getting bigger while multiplying by the next number in the 
 *	sequence. 
 *
 *      The new algorithm takes advantage of the fast multiplication 
 *	algorithm. The "ideal" setup for fast multiplication is when 
 *	both numbers have approx the same number of significant digits 
 *	and the number of digits is very near (but not over) an exact 
 *	power of 2.
 *
 *	So, we will multiply N * (N-1) * (N-2), etc until the number of
 *	significant digits is approx 256.
 *
 *	Store this temp product into an array.
 *
 *	Then we will multiply the next sequence until the number of
 *	significant digits is approx 256.
 *
 *	Store this temp product into the next element of the array.
 *
 *	Continue until we've counted down to 2.
 *
 *	We now have an array of numbers with approx the same number
 *	of digits (except for the last element, depending on where it 
 *	ended.) Now multiply each of the array elements together to
 *	get the final product.
 *
 *      The array multiplies are done as follows (assume we used 11
 *	array elements for this example, indicated by [0] - [10] ) :
 *
 *	initial    iter-1     iter-2       iter-3     iter-4
 *
 *	  [0] 
 *	     *  ->  [0]
 *	  [1]
 *                      * ->    [0]
 *
 *	  [2] 
 *	     *  ->  [1]
 *	  [3]
 *                                   * ->   [0] 
 *
 *	  [4] 
 *	     *  ->  [2]
 *	  [5]
 *
 *                      * ->    [1]
 *
 *	  [6] 
 *	     *  ->  [3]                           *  ->  [0]
 *	  [7]
 *
 *
 *	  [8] 
 *	     *  ->  [4]
 *	  [9]
 *                      * ->    [2]    ->   [1]
 *
 *
 *	  [10]  ->  [5]
 *
 */

#include "m_apm_lc.h"

/* define size of local array for temp storage */

#define NDIM 32

/****************************************************************************/
void	m_apm_factorial(M_APM moutput, M_APM minput)
{
int     ii, nmul, ndigits, nd, jj, kk, mm, ct;
M_APM   array[NDIM];
M_APM   iprod1, iprod2, tmp1, tmp2;

/* return 1 for any input <= 1 */

if (m_apm_compare(minput, MM_One) <= 0)
  {
   m_apm_copy(moutput, MM_One);
   return;
  }

ct       = 0;
mm       = NDIM - 2;
ndigits  = 256;
nd       = ndigits - 20;
tmp1     = m_apm_init();
tmp2     = m_apm_init();
iprod1   = m_apm_init();
iprod2   = m_apm_init();
array[0] = m_apm_init();

m_apm_copy(tmp2, minput);

/* loop until multiply count-down has reached '2' */

while (TRUE)
  {
   m_apm_copy(iprod1, MM_One);

   /* 
    *   loop until the number of significant digits in this 
    *   partial result is slightly less than 256
    */

   while (TRUE)
     {
      m_apm_multiply(iprod2, iprod1, tmp2);

      m_apm_subtract(tmp1, tmp2, MM_One);

      m_apm_multiply(iprod1, iprod2, tmp1);

      /*
       *  I know, I know.  There just isn't a *clean* way 
       *  to break out of 2 nested loops.
       */

      if (m_apm_compare(tmp1, MM_Two) <= 0)
        goto PHASE2;

      m_apm_subtract(tmp2, tmp1, MM_One);

      if (iprod1->m_apm_datalength > nd)
        break;
     }

   if (ct == (NDIM - 1))
     {
      /* 
       *    if the array has filled up, start multiplying
       *    some of the partial products now.
       */

      m_apm_copy(tmp1, array[mm]);
      m_apm_multiply(array[mm], iprod1, tmp1);

      if (mm == 0)
        {
         mm = NDIM - 2;
	 ndigits = ndigits << 1;
         nd = ndigits - 20;
	}
      else
         mm--;
     }
   else
     {
      /* 
       *    store this partial product in the array
       *    and allocate the next array element
       */

      m_apm_copy(array[ct], iprod1);
      array[++ct] = m_apm_init();
     }
  }

PHASE2:

m_apm_copy(array[ct], iprod1);

kk = ct;

while (kk != 0)
  {
   ii = 0;
   jj = 0;
   nmul = (kk + 1) >> 1;

   while (TRUE)
     {
      /* must use tmp var when ii,jj point to same element */

      if (ii == 0)
        {
         m_apm_copy(tmp1, array[ii]);
         m_apm_multiply(array[jj], tmp1, array[ii+1]);
        }
      else
         m_apm_multiply(array[jj], array[ii], array[ii+1]);

      if (++jj == nmul)
        break;

      ii += 2;
     }

   if ((kk & 1) == 0)
     {
      jj = kk >> 1;
      m_apm_copy(array[jj], array[kk]);
     }

   kk = kk >> 1;
  }

m_apm_copy(moutput, array[0]);

for (ii=0; ii <= ct; ii++)
  {
   m_apm_free(array[ii]);
  }

m_apm_free(tmp1);
m_apm_free(tmp2);
m_apm_free(iprod1);
m_apm_free(iprod2);
}
/****************************************************************************/
