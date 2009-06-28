
/* 
 *  M_APM  -  mapmutl2.c
 *
 *  Copyright (C) 2002 - 2007   Michael C. Ring
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
 *      $Id: mapmutl2.c,v 1.7 2007/12/03 02:00:04 mike Exp $
 *
 *      This file contains various utility functions
 *
 *      $Log: mapmutl2.c,v $
 *      Revision 1.7  2007/12/03 02:00:04  mike
 *      Update license
 *
 *      Revision 1.6  2003/07/21 20:53:10  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.5  2003/05/04 18:14:32  mike
 *      move generic error handling function into a dedicated module
 *
 *      Revision 1.4  2003/03/31 22:02:22  mike
 *      call generic error handling function
 *
 *      Revision 1.3  2002/11/03 21:19:40  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.2  2002/05/17 22:29:46  mike
 *      update some comments
 *
 *      Revision 1.1  2002/05/17 22:28:27  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
int	m_apm_sign(M_APM atmp)
{
return(atmp->m_apm_sign);
}
/****************************************************************************/
int	m_apm_exponent(M_APM atmp)
{
if (atmp->m_apm_sign == 0)
  return(0);
else
  return(atmp->m_apm_exponent - 1);
}
/****************************************************************************/
int	m_apm_significant_digits(M_APM atmp)
{
return(atmp->m_apm_datalength);
}
/****************************************************************************/
int	m_apm_is_integer(M_APM atmp)
{
if (atmp->m_apm_sign == 0)
  return(1);

if (atmp->m_apm_exponent >= atmp->m_apm_datalength)
  return(1);
else
  return(0);
}
/****************************************************************************/
int 	m_apm_is_even(M_APM aa)
{
int     ii, jj;

if (aa->m_apm_sign == 0)
  return(1);

ii = aa->m_apm_datalength;
jj = aa->m_apm_exponent;

if (jj < ii)
  {
   M_apm_log_error_msg(M_APM_RETURN, "\'m_apm_is_even\', Non-integer input");
   return(0);
  }

if (jj > ii)
  return(1);

ii = ((ii + 1) >> 1) - 1;
ii = (int)aa->m_apm_data[ii];

if ((jj & 1) != 0)      /* exponent is odd */
  ii = ii / 10;

if ((ii & 1) == 0)
  return(1);
else
  return(0);
}
/****************************************************************************/
int 	m_apm_is_odd(M_APM bb)
{
if (m_apm_is_even(bb))
  return(0);
else
  return(1);
}
/****************************************************************************/
void	M_set_to_zero(M_APM z)
{
z->m_apm_datalength = 1;
z->m_apm_sign       = 0;
z->m_apm_exponent   = 0;
z->m_apm_data[0]    = 0;
}
/****************************************************************************/
void	m_apm_negate(M_APM d, M_APM s)
{
m_apm_copy(d,s);
if (d->m_apm_sign != 0)
    d->m_apm_sign = -(d->m_apm_sign);
}
/****************************************************************************/
void	m_apm_absolute_value(M_APM d, M_APM s)
{
m_apm_copy(d,s);
if (d->m_apm_sign != 0)
    d->m_apm_sign = 1;
}
/****************************************************************************/
void	m_apm_copy(M_APM dest, M_APM src)
{
int	j;
void	*vp;

j = (src->m_apm_datalength + 1) >> 1;
if (j > dest->m_apm_malloclength)
  {
   if ((vp = MAPM_REALLOC(dest->m_apm_data, (j + 32))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'m_apm_copy\', Out of memory");
     }
   
   dest->m_apm_malloclength = j + 28;
   dest->m_apm_data = (UCHAR *)vp;
  }

dest->m_apm_datalength = src->m_apm_datalength;
dest->m_apm_exponent   = src->m_apm_exponent;
dest->m_apm_sign       = src->m_apm_sign;

memcpy(dest->m_apm_data, src->m_apm_data, j);
}
/****************************************************************************/
int	m_apm_compare(M_APM ltmp, M_APM rtmp)
{
int	llen, rlen, lsign, rsign, i, j, lexp, rexp;

llen  = ltmp->m_apm_datalength;
rlen  = rtmp->m_apm_datalength;

lsign = ltmp->m_apm_sign;
rsign = rtmp->m_apm_sign;

lexp  = ltmp->m_apm_exponent;
rexp  = rtmp->m_apm_exponent;

if (rsign == 0)
  return(lsign);

if (lsign == 0)
  return(-rsign);

if (lsign == -rsign)
  return(lsign);

/* signs are the same, check the exponents */

if (lexp > rexp)
  goto E1;

if (lexp < rexp)
  goto E2;

/* signs and exponents are the same, check the data */

if (llen < rlen)
  j = (llen + 1) >> 1;
else
  j = (rlen + 1) >> 1;

for (i=0; i < j; i++)
  {
   if (ltmp->m_apm_data[i] > rtmp->m_apm_data[i])
     goto E1;

   if (ltmp->m_apm_data[i] < rtmp->m_apm_data[i])
     goto E2;
  }

if (llen == rlen)
   return(0);
else
  {
   if (llen > rlen)
     goto E1;
   else
     goto E2;
  }

E1:

if (lsign == 1)
  return(1);
else
  return(-1);

E2:

if (lsign == 1)
  return(-1);
else
  return(1);
}
/****************************************************************************/
/*
 *
 *	convert a signed long int to ASCII in base 10
 *
 */
void    M_long_2_ascii(char *output, long input)
{
long    t, m;
int     i, j;
char    *p, tbuf[64];

m = input;
p = output;
i = 0;
t = 2147000000L;          /* something < 2^31 */

if ((m > t) || (m < -t))  /* handle the bigger numbers with 'sprintf'. */
  {			  /* let them worry about wrap-around problems */
   sprintf(p, "%ld", m);  /* at 'LONG_MIN', etc.                       */
  }
else
  {
   if (m < 0)             /* handle the sign */
     {
      *p++ = '-';
      m = -m;
     }
   
   while (TRUE)           /* build the digits in reverse order */
     {
      t = m / 10;
      j = (int)(m - (10 * t));
      tbuf[i++] = (char)(j + '0');
      m = t;

      if (t == 0)  
        break;
     }
   
   while (TRUE)           /* fill output string in the correct order */
     {
      *p++ = tbuf[--i];
      if (i == 0)  
        break;
     }
   
   *p = '\0';
  }
}
/****************************************************************************/
/*
 *      this function will convert a string to lowercase
 */
char    *M_lowercase(char *s)
{
char    *p;

p = s;

while (TRUE)
  {
   if (*p >= 'A' && *p <= 'Z')
     *p += 'a' - 'A';

   if (*p++ == '\0')  break;
  }
return(s);
}
/****************************************************************************/
/*    returns char position of first occurence of s2 in s1
	  or -1 if no match found
*/
int     M_strposition(char *s1, char *s2)
{
register char  ch1, ch2;
char           *p0, *p1, *p2;
int            ct;

ct = -1;
p0 = s1;

if (*s2 == '\0')  return(-1);

while (TRUE)
  {
   ct++;
   p1  = p0;
   p2  = s2;
   ch2 = *p2;
   
   while (TRUE)                    /* scan until first char matches */
     {
      if ((ch1 = *p1) == '\0')  return(-1);
      if (ch1 == ch2)           break;
      p1++;
      ct++;
     }

   p2++;                           /* check remainder of 2 strings */
   p1++;
   p0 = p1;

   while (TRUE)
     {
      if ((ch2 = *p2) == '\0')  return(ct);
      if (*p1 != ch2)           break;
      p1++;
      p2++;
     }
  }
}
/****************************************************************************/
