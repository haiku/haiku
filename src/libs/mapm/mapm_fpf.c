
/* 
 *  M_APM  -  mapm_fpf.c
 *
 *  Copyright (C) 2001 - 2007   Michael C. Ring
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
 *      $Id: mapm_fpf.c,v 1.10 2007/12/03 01:39:57 mike Exp $
 *
 *      This file contains the Fixed Point Formatting functions
 *
 *      $Log: mapm_fpf.c,v $
 *      Revision 1.10  2007/12/03 01:39:57  mike
 *      Update license
 *
 *      Revision 1.9  2003/07/21 20:15:12  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.8  2003/03/31 22:11:14  mike
 *      call generic error handling function
 *
 *      Revision 1.7  2002/11/05 23:31:00  mike
 *      use new set_to_zero call instead of copy
 *
 *      Revision 1.6  2002/11/03 22:33:24  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.5  2002/02/14 19:31:44  mike
 *      eliminate need for conditional compile
 *
 *      Revision 1.4  2001/08/26 22:35:50  mike
 *      no LCC conditional needed on fixpt_string
 *
 *      Revision 1.3  2001/08/26 22:11:10  mike
 *      add new 'stringexp' function
 *
 *      Revision 1.2  2001/08/25 22:30:09  mike
 *      fix LCC-WIN32 compile problem
 *
 *      Revision 1.1  2001/08/25 16:50:59  mike
 *      Initial revision
 */

#include "m_apm_lc.h"
#include <ctype.h>

/****************************************************************************/
char	*m_apm_to_fixpt_stringexp(int dplaces, M_APM atmp, 
				  char ch_radx, char ch_sep, int ct_sep)
{
int	places, xp, dl, ii;
char	*cpr;

places = dplaces;

dl = atmp->m_apm_datalength;
xp = atmp->m_apm_exponent;

if (places < 0)				/* show ALL digits */
  {
   if (xp < 0)
      ii = dl - xp;
   else
     {
      if (dl > xp)
        ii = dl;
      else
        ii = xp;
     }
  }
else
  {
   ii = places;
      
   if (xp > 0)
     ii += xp;
  }

if (ct_sep != 0 && ch_sep != 0 && xp > 0)
  ii += xp / ct_sep;

if ((cpr = (char *)MAPM_MALLOC((ii + 32) * sizeof(char))) == NULL)
  return(NULL);

m_apm_to_fixpt_stringex(cpr,places,atmp,ch_radx,ch_sep,ct_sep);

return(cpr);
}
/****************************************************************************/
void	m_apm_to_fixpt_stringex(char *s, int dplaces, M_APM atmp, 
				char ch_radix, char ch_sep, int count_sep)
{
M_APM   btmp;
char    ch, *cpd, *cps;
int	ii, jj, kk, ct, dl, xp, no_sep_flg, places;

btmp       = M_get_stack_var();
places     = dplaces;
cpd        = s;
no_sep_flg = FALSE;

m_apm_absolute_value(btmp, atmp);	/* do conversion of positive number */

if (ch_sep == 0 || count_sep == 0)	/* no separator char OR count */
  no_sep_flg = TRUE;

/* determine how much memory to get for the temp string */

dl = btmp->m_apm_datalength;
xp = btmp->m_apm_exponent;

if (places < 0)				/* show ALL digits */
  {
   if (xp < 0)
      ii = dl - xp;
   else
     {
      if (dl > xp)
        ii = dl;
      else
        ii = xp;
     }
  }
else
  {
   ii = places;
      
   if (xp > 0)
     ii += xp;
  }

if ((cps = (char *)MAPM_MALLOC((ii + 32) * sizeof(char))) == NULL)
  {
   /* fatal, this does not return */

   M_apm_log_error_msg(M_APM_FATAL, 
                       "\'m_apm_to_fixpt_stringex\', Out of memory");
  }

m_apm_to_fixpt_string(cps, places, btmp);

/*
 *  the converted string may be all 'zero', 0.0000...
 *  if so and the original number is negative,
 *  do NOT set the '-' sign of our output string.
 */

if (atmp->m_apm_sign == -1)		/* if input number negative */
  {
   kk = 0;
   jj = 0;

   while (TRUE)
     {
      ch = cps[kk++];
      if ((ch == '\0') || (jj != 0))
        break;

      if (isdigit((int)ch))
        {
	 if (ch != '0')
	   jj = 1;
	}
     }

   if (jj)
     *cpd++ = '-';
  }

ct = M_strposition(cps, ".");      /* find the default (.) radix char */

if (ct == -1)			   /* if not found .. */
  {
   strcat(cps, ".");               /* add one */
   ct = M_strposition(cps, ".");   /* and then find it */
  }

if (places == 0)		   /* int format, terminate at radix char */
  cps[ct] = '\0';
else
  cps[ct] = ch_radix;		   /* assign the radix char */

/*
 *  if the number is small enough to not have any separator char's ...
 */

if (ct <= count_sep)
  no_sep_flg = TRUE;

if (no_sep_flg)
  {
   strcpy(cpd, cps);
  }
else
  {
   jj = 0;
   kk = count_sep;
   ii = ct / count_sep;

   if ((ii = ct - ii * count_sep) == 0)
     ii = count_sep;

   while (TRUE)				/* write out the first 1,2  */
     {					/* (up to count_sep) digits */
      *cpd++ = cps[jj++];

      if (--ii == 0)
        break;
     }

   while (TRUE)				/* write rest of the string   */
     {
      if (kk == count_sep)		/* write a new separator char */
        {
	 if (jj != ct)			/* unless we're at the radix  */
	   {
            *cpd++ = ch_sep;		/* note that this also disables */
	    kk = 0;			/* the separator char AFTER     */
	   }				/* the radix char               */
	}

      if ((*cpd++ = cps[jj++]) == '\0')
        break;

      kk++;
     }
  }

MAPM_FREE(cps);
M_restore_stack(1);
}
/****************************************************************************/
void	m_apm_to_fixpt_string(char *ss, int dplaces, M_APM mtmp)
{
M_APM   ctmp;
void	*vp;
int	places, i2, ii, jj, kk, xp, dl, numb;
UCHAR   *ucp, numdiv, numrem;
char	*cpw, *cpd, sbuf[128];

ctmp   = M_get_stack_var();
vp     = NULL;
cpd    = ss;
places = dplaces;

/* just want integer portion if places == 0 */

if (places == 0)
  {
   if (mtmp->m_apm_sign >= 0)
     m_apm_add(ctmp, mtmp, MM_0_5);
   else
     m_apm_subtract(ctmp, mtmp, MM_0_5);

   m_apm_to_integer_string(cpd, ctmp);

   M_restore_stack(1);
   return;
  }

if (places > 0)
  M_apm_round_fixpt(ctmp, places, mtmp);
else
  m_apm_copy(ctmp, mtmp);	  /* show ALL digits */

if (ctmp->m_apm_sign == 0)        /* result is 0 */
  {
   if (places < 0)
     {
      cpd[0] = '0';		  /* "0.0" */
      cpd[1] = '.';
      cpd[2] = '0';
      cpd[3] = '\0';
     }
   else
     {
      memset(cpd, '0', (places + 2));	/* pre-load string with all '0' */
      cpd[1] = '.';
      cpd[places + 2] = '\0';
     }

   M_restore_stack(1);
   return;
  }

xp   = ctmp->m_apm_exponent;
dl   = ctmp->m_apm_datalength;
numb = (dl + 1) >> 1;

if (places < 0)
  {
   if (dl > xp)
     jj = dl + 16;
   else
     jj = xp + 16;
  }
else
  {
   jj = places + 16;
   
   if (xp > 0)
     jj += xp;
  }

if (jj > 112)
  {
   if ((vp = (void *)MAPM_MALLOC((jj + 16) * sizeof(char))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, 
                          "\'m_apm_to_fixpt_string\', Out of memory");
     }

   cpw = (char *)vp;
  }
else
  {
   cpw = sbuf;
  }

/*
 *  at this point, the number is non-zero and the the output
 *  string will contain at least 1 significant digit.
 */

if (ctmp->m_apm_sign == -1) 	  /* negative number */
  {
   *cpd++ = '-';
  }

ucp = ctmp->m_apm_data;
ii  = 0;

/* convert MAPM num to ASCII digits and store in working char array */

while (TRUE)
  {
   M_get_div_rem_10((int)(*ucp++), &numdiv, &numrem);

   cpw[ii++] = numdiv + '0';
   cpw[ii++] = numrem + '0';

   if (--numb == 0)
     break;
  }

i2 = ii;		/* save for later */

if (places < 0)		/* show ALL digits */
  {
   places = dl - xp;

   if (places < 1)
     places = 1;
  }

/* pad with trailing zeros if needed */

kk = xp + places + 2 - ii;

if (kk > 0)
  memset(&cpw[ii], '0', kk);

if (xp > 0)          /* |num| >= 1, NO lead-in "0.nnn" */
  {
   ii = xp + places + 1;
   jj = 0;

   for (kk=0; kk < ii; kk++)
     {
      if (kk == xp)
        cpd[jj++] = '.';

      cpd[jj++] = cpw[kk];
     }

   cpd[ii] = '\0';
  }
else			/* |num| < 1, have lead-in "0.nnn" */
  {
   jj = 2 - xp;
   ii = 2 + places;
   memset(cpd, '0', (ii + 1));	/* pre-load string with all '0' */
   cpd[1] = '.';		/* assign decimal point */

   for (kk=0; kk < i2; kk++)
     {
      cpd[jj++] = cpw[kk];
     }

   cpd[ii] = '\0';
  }

if (vp != NULL)
  MAPM_FREE(vp);

M_restore_stack(1);
}
/****************************************************************************/
void	M_apm_round_fixpt(M_APM btmp, int places, M_APM atmp)
{
int	xp, ii;

xp = atmp->m_apm_exponent;
ii = xp + places - 1;

M_set_to_zero(btmp); /* assume number is too small so the net result is 0 */

if (ii >= 0)
  {
   m_apm_round(btmp, ii, atmp);
  }
else
  {
   if (ii == -1)	/* next digit is significant which may round up */
     {
      if (atmp->m_apm_data[0] >= 50)	/* digit >= 5, round up */
        {
         m_apm_copy(btmp, atmp);
	 btmp->m_apm_data[0] = 10;
	 btmp->m_apm_exponent += 1;
	 btmp->m_apm_datalength = 1;
	 M_apm_normalize(btmp);
	}
     }
  }
}
/****************************************************************************/

