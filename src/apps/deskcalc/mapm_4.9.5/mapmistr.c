
/* 
 *  M_APM  -  mapmistr.c
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
 *      $Id: mapmistr.c,v 1.9 2007/12/03 01:55:27 mike Exp $
 *
 *      This file contains M_APM -> integer string function
 *
 *      $Log: mapmistr.c,v $
 *      Revision 1.9  2007/12/03 01:55:27  mike
 *      Update license
 *
 *      Revision 1.8  2003/07/21 20:37:09  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.7  2003/03/31 21:52:07  mike
 *      call generic error handling function
 *
 *      Revision 1.6  2002/11/03 22:28:02  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.5  2001/08/06 16:58:20  mike
 *      improve the new function
 *
 *      Revision 1.4  2001/08/05 23:18:48  mike
 *      fix function when input is not an integer but the
 *      number is close to rounding upwards (NNN.9999999999....)
 *
 *      Revision 1.3  2000/02/03 22:48:38  mike
 *      use MAPM_* generic memory function
 *
 *      Revision 1.2  1999/07/18 01:33:04  mike
 *      minor tweak to code alignment
 *
 *      Revision 1.1  1999/07/12 02:06:08  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
void	m_apm_to_integer_string(char *s, M_APM mtmp)
{
void    *vp;
UCHAR	*ucp, numdiv, numrem;
char	*cp, *p, sbuf[128];
int	ct, dl, numb, ii;

vp = NULL;
ct = mtmp->m_apm_exponent;
dl = mtmp->m_apm_datalength;

/*
 *  if |input| < 1, result is "0"
 */

if (ct <= 0 || mtmp->m_apm_sign == 0)
  {
   s[0] = '0';
   s[1] = '\0';
   return;
  }

if (ct > 112)
  {
   if ((vp = (void *)MAPM_MALLOC((ct + 32) * sizeof(char))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, 
                          "\'m_apm_to_integer_string\', Out of memory");
     }

   cp = (char *)vp;
  }
else
  {
   cp = sbuf;
  }

p  = cp;
ii = 0;

/* handle a negative number */

if (mtmp->m_apm_sign == -1)
  {
   ii = 1;
   *p++ = '-';
  }

/* get num-bytes of data (#digits / 2) to use in the string */

if (ct > dl)
  numb = (dl + 1) >> 1;
else
  numb = (ct + 1) >> 1;

ucp = mtmp->m_apm_data;

while (TRUE)
  {
   M_get_div_rem_10((int)(*ucp++), &numdiv, &numrem);

   *p++ = numdiv + '0';
   *p++ = numrem + '0';

   if (--numb == 0)
     break;
  }

/* pad with trailing zeros if the exponent > datalength */
 
if (ct > dl)
  memset(p, '0', (ct + 1 - dl));

cp[ct + ii] = '\0';
strcpy(s, cp);

if (vp != NULL)
  MAPM_FREE(vp);
}
/****************************************************************************/
