
/* 
 *  M_APM  -  mapmutil.c
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
 *      $Id: mapmutil.c,v 1.26 2007/12/03 01:58:49 mike Exp $
 *
 *      This file contains various utility functions needed by the 
 *	library in addition to some basic user callable functions.
 *
 *      $Log: mapmutil.c,v $
 *      Revision 1.26  2007/12/03 01:58:49  mike
 *      Update license
 *
 *      Revision 1.25  2003/07/21 20:51:34  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.24  2003/03/31 22:03:54  mike
 *      call generic error handling function
 *
 *      Revision 1.23  2002/11/04 20:47:02  mike
 *      change m_apm_init so it compiles clean with a real C++ compiler
 *
 *      Revision 1.22  2002/11/03 22:50:58  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.21  2002/05/17 22:26:49  mike
 *      move some functions into another file
 *
 *      Revision 1.20  2002/02/12 20:21:53  mike
 *      eliminate unneeded working arrays in _scale
 *      by processing the scaling operation in reverse
 *
 *      Revision 1.19  2001/07/24 18:29:18  mike
 *      add util function to get address of
 *      the div/rem lookup tables
 *
 *      Revision 1.18  2001/07/20 16:14:05  mike
 *      optimize normalize yet again
 *
 *      Revision 1.17  2001/07/17 18:17:56  mike
 *      another optimization to _normalize
 *
 *      Revision 1.16  2001/07/16 22:33:43  mike
 *      update free_all_util
 *
 *      Revision 1.15  2001/07/16 19:56:26  mike
 *      add function M_free_all_util
 *
 *      Revision 1.14  2001/07/16 18:10:21  mike
 *      optimize M_apm_normalize when moving multiple '00' bytes
 *
 *      Revision 1.13  2001/02/11 22:36:43  mike
 *      modify parameters to REALLOC
 *
 *      Revision 1.12  2001/01/23 21:17:38  mike
 *      add dedicated long->ascii conversion (instead of sprintf)
 *
 *      Revision 1.11  2000/08/22 20:21:54  mike
 *      fix m_apm_exponent with exactly 0 as the input
 *
 *      Revision 1.10  2000/08/22 00:01:26  mike
 *      add zero check in is_integer
 *
 *      Revision 1.9  2000/08/21 23:34:44  mike
 *      add new function _is_integer
 *
 *      Revision 1.8  2000/08/01 22:29:02  mike
 *      add sizeof int function call
 *
 *      Revision 1.7  2000/05/19 16:21:03  mike
 *      delete M_check_dec_places, no longer needed
 *
 *      Revision 1.6  2000/04/04 17:06:37  mike
 *      initialize C++ refcount struct element to 1
 *
 *      Revision 1.5  2000/02/03 22:49:56  mike
 *      use MAPM_* generic memory function
 *
 *      Revision 1.4  1999/09/18 03:06:41  mike
 *      fix m_apm_exponent
 *
 *      Revision 1.3  1999/09/18 02:59:11  mike
 *      added new functions
 *
 *      Revision 1.2  1999/05/15 02:21:14  mike
 *      add check for number of decimal places
 *
 *      Revision 1.1  1999/05/10 20:56:31  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

static  UCHAR	*M_mul_div = NULL;
static  UCHAR   *M_mul_rem = NULL;

static  UCHAR   M_mul_div_10[100];
static	UCHAR   M_mul_rem_10[100];

static	int	M_util_firsttime = TRUE;
static	int     M_firsttime3 = TRUE;

static	M_APM	M_work_0_5;

static  char    *M_init_error_msg = "\'m_apm_init\', Out of memory";

/****************************************************************************/
M_APM	m_apm_init()
{
M_APM	atmp;

if (M_firsttime3)
  {
   M_firsttime3 = FALSE;
   M_init_util_data();
   M_init_trig_globals();
  }

if ((atmp = (M_APM)MAPM_MALLOC(sizeof(M_APM_struct))) == NULL)
  {
   /* fatal, this does not return */

   M_apm_log_error_msg(M_APM_FATAL, M_init_error_msg);
  }

atmp->m_apm_id           = M_APM_IDENT;
atmp->m_apm_malloclength = 80;
atmp->m_apm_datalength   = 1;
atmp->m_apm_refcount     = 1;           /* not for us, for MAPM C++ class */
atmp->m_apm_exponent     = 0;
atmp->m_apm_sign         = 0;

if ((atmp->m_apm_data = (UCHAR *)MAPM_MALLOC(84)) == NULL)
  {
   /* fatal, this does not return */

   M_apm_log_error_msg(M_APM_FATAL, M_init_error_msg);
  }

atmp->m_apm_data[0] = 0;
return(atmp);
}
/****************************************************************************/
void	m_apm_free(M_APM atmp)
{
if (atmp->m_apm_id == M_APM_IDENT)
  {
   atmp->m_apm_id = 0x0FFFFFF0L;
   MAPM_FREE(atmp->m_apm_data);
   MAPM_FREE(atmp);
  }
else
  {
   M_apm_log_error_msg(M_APM_RETURN, "\'m_apm_free\', Invalid M_APM variable");
  }
}
/****************************************************************************/
void	M_free_all_util()
{
if (M_util_firsttime == FALSE)
  {
   m_apm_free(M_work_0_5);
   M_util_firsttime = TRUE;
  }

if (M_firsttime3 == FALSE)
  {
   MAPM_FREE(M_mul_div);
   MAPM_FREE(M_mul_rem);
  
   M_mul_div    = NULL;
   M_mul_rem    = NULL;
   M_firsttime3 = TRUE;
  }
}
/****************************************************************************/
/*
 *      just a dummy wrapper to keep some compilers from complaining
 */
int 	M_get_sizeof_int()
{
return(sizeof(int));
}
/****************************************************************************/
void	M_init_util_data()
{
int	k;
UCHAR   ndiv, nrem;

if (M_mul_div != NULL)
  return;

M_mul_div = (UCHAR *)MAPM_MALLOC(10000 * sizeof(UCHAR));
M_mul_rem = (UCHAR *)MAPM_MALLOC(10000 * sizeof(UCHAR));

if (M_mul_div == NULL || M_mul_rem == NULL)
  {
   /* fatal, this does not return */

   M_apm_log_error_msg(M_APM_FATAL, "\'M_init_util_data\', Out of memory");
  }

ndiv = 0;
nrem = 0;

for (k=0; k < 100; k++)
  {
   M_mul_div_10[k] = ndiv;
   M_mul_rem_10[k] = nrem;

   if (++nrem == 10)
     {
      nrem = 0;
      ndiv++;
     }
  }

ndiv = 0;
nrem = 0;

for (k=0; k < 10000; k++)
  {
   M_mul_div[k] = ndiv;
   M_mul_rem[k] = nrem;

   if (++nrem == 100)
     {
      nrem = 0;
      ndiv++;
     }
  }
}
/****************************************************************************/
void	M_get_div_rem_addr(UCHAR **ndivp, UCHAR **nremp)
{
*ndivp = M_mul_div;
*nremp = M_mul_rem;
}
/****************************************************************************/
void	M_get_div_rem(int tbl_lookup, UCHAR *ndiv, UCHAR *nrem)
{
*ndiv = M_mul_div[tbl_lookup];
*nrem = M_mul_rem[tbl_lookup];
}
/****************************************************************************/
void	M_get_div_rem_10(int tbl_lookup, UCHAR *ndiv, UCHAR *nrem)
{
*ndiv = M_mul_div_10[tbl_lookup];
*nrem = M_mul_rem_10[tbl_lookup];
}
/****************************************************************************/
void	m_apm_round(M_APM btmp, int places, M_APM atmp) 
{
int	ii;

if (M_util_firsttime)
  {
   M_util_firsttime = FALSE;

   M_work_0_5 = m_apm_init();
   m_apm_set_string(M_work_0_5, "5");
  }

ii = places + 1;

if (atmp->m_apm_datalength <= ii)
  {
   m_apm_copy(btmp,atmp);
   return;
  }

M_work_0_5->m_apm_exponent = atmp->m_apm_exponent - ii;

if (atmp->m_apm_sign > 0)
  m_apm_add(btmp, atmp, M_work_0_5);
else
  m_apm_subtract(btmp, atmp, M_work_0_5);

btmp->m_apm_datalength = ii;
M_apm_normalize(btmp);
}
/****************************************************************************/
void	M_apm_normalize(M_APM atmp)
{
int	i, index, datalength, exponent;
UCHAR   *ucp, numdiv, numrem, numrem2;

if (atmp->m_apm_sign == 0)
  return;

datalength = atmp->m_apm_datalength;
exponent   = atmp->m_apm_exponent;

/* make sure trailing bytes/chars are 0                */
/* the following function will adjust the 'datalength' */
/* we want the original value and will fix it later    */

M_apm_pad(atmp, (datalength + 3));

while (TRUE)			/* remove lead-in '0' if any */
  {
   M_get_div_rem_10((int)atmp->m_apm_data[0], &numdiv, &numrem);

   if (numdiv >= 1)      /* number is normalized, done here */
     break;

   index = (datalength + 1) >> 1;

   if (numrem == 0)      /* both nibbles are 0, we can move full bytes */
     {
      i = 0;
      ucp = atmp->m_apm_data;

      while (TRUE)	 /* find out how many '00' bytes we can move */
        {
	 if (*ucp != 0)
	   break;

         ucp++;
	 i++;
	}

      memmove(atmp->m_apm_data, ucp, (index + 1 - i));
      datalength -= 2 * i;
      exponent -= 2 * i;
     }
   else
     {
      for (i=0; i < index; i++)
        {
         M_get_div_rem_10((int)atmp->m_apm_data[i+1], &numdiv, &numrem2);
         atmp->m_apm_data[i] = 10 * numrem + numdiv;
	 numrem = numrem2;
        }
   
      datalength--;
      exponent--;
     }
  }

while (TRUE)			/* remove trailing '0' if any */
  {
   index = ((datalength + 1) >> 1) - 1;

   if ((datalength & 1) == 0)   /* back-up full bytes at a time if the */
     {				/* current length is an even number    */
      ucp = atmp->m_apm_data + index;
      if (*ucp == 0)
        {
	 while (TRUE)
	   {
	    datalength -= 2;
	    index--;
	    ucp--;

	    if (*ucp != 0)
	      break;
	   }
	}
     }

   M_get_div_rem_10((int)atmp->m_apm_data[index], &numdiv, &numrem);

   if (numrem != 0)		/* last digit non-zero, all done */
     break;

   if ((datalength & 1) != 0)   /* if odd, then first char must be non-zero */
     {
      if (numdiv != 0)
        break;
     }

   if (datalength == 1)
     {
      atmp->m_apm_sign = 0;
      exponent = 0;
      break;
     }
     
   datalength--;
  }

atmp->m_apm_datalength = datalength;
atmp->m_apm_exponent   = exponent;
}
/****************************************************************************/
void	M_apm_scale(M_APM ctmp, int count)
{
int	ii, numb, ct;
UCHAR	*chp, numdiv, numdiv2, numrem;
void	*vp;

ct = count;

ii = (ctmp->m_apm_datalength + ct + 1) >> 1;
if (ii > ctmp->m_apm_malloclength)
  {
   if ((vp = MAPM_REALLOC(ctmp->m_apm_data, (ii + 32))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'M_apm_scale\', Out of memory");
     }
   
   ctmp->m_apm_malloclength = ii + 28;
   ctmp->m_apm_data = (UCHAR *)vp;
  }

if ((ct & 1) != 0)          /* move odd number first */
  {
   ct--;
   chp = ctmp->m_apm_data;
   ii  = ((ctmp->m_apm_datalength + 1) >> 1) - 1;

   if ((ctmp->m_apm_datalength & 1) == 0)
     {
      /*
       *   original datalength is even:
       *
       *   uv  wx  yz   becomes  -->   0u  vw  xy  z0
       */

      numdiv = 0;

      while (TRUE)
        {
         M_get_div_rem_10((int)chp[ii], &numdiv2, &numrem);

	 chp[ii + 1] = 10 * numrem + numdiv;
	 numdiv = numdiv2;

	 if (ii == 0)
	   break;

         ii--;
	}

      chp[0] = numdiv2;
     }
   else
     {
      /*
       *   original datalength is odd:
       *
       *   uv  wx  y0   becomes  -->   0u  vw  xy
       */

      M_get_div_rem_10((int)chp[ii], &numdiv2, &numrem);

      if (ii == 0)
        {
         chp[0] = numdiv2;
        }
      else
        {
         while (TRUE)
           {
            M_get_div_rem_10((int)chp[ii - 1], &numdiv, &numrem);

	    chp[ii] = 10 * numrem + numdiv2;
	    numdiv2 = numdiv;

	    if (--ii == 0)
	      break;
	   }

         chp[0] = numdiv;
        }
     }

   ctmp->m_apm_exponent++;
   ctmp->m_apm_datalength++;
  }

/* ct is even here */

if (ct > 0)
  {
   numb = (ctmp->m_apm_datalength + 1) >> 1;
   ii   = ct >> 1;
   
   memmove((ctmp->m_apm_data + ii), ctmp->m_apm_data, numb);
   memset(ctmp->m_apm_data, 0, ii);
   
   ctmp->m_apm_datalength += ct;
   ctmp->m_apm_exponent += ct;
  }
}
/****************************************************************************/
void	M_apm_pad(M_APM ctmp, int new_length)
{
int	num1, numb, ct;
UCHAR	numdiv, numrem;
void	*vp;

ct = new_length;
if (ctmp->m_apm_datalength >= ct)
  return;
  
numb = (ct + 1) >> 1;
if (numb > ctmp->m_apm_malloclength)
  {
   if ((vp = MAPM_REALLOC(ctmp->m_apm_data, (numb + 32))) == NULL)
     {
      /* fatal, this does not return */

      M_apm_log_error_msg(M_APM_FATAL, "\'M_apm_pad\', Out of memory");
     }
   
   ctmp->m_apm_malloclength = numb + 28;
   ctmp->m_apm_data = (UCHAR *)vp;
  }

num1 = (ctmp->m_apm_datalength + 1) >> 1;

if ((ctmp->m_apm_datalength & 1) != 0)
  {
   M_get_div_rem_10((int)ctmp->m_apm_data[num1 - 1], &numdiv, &numrem);
   ctmp->m_apm_data[num1 - 1] = 10 * numdiv;
  }

memset((ctmp->m_apm_data + num1), 0, (numb - num1));
ctmp->m_apm_datalength = ct;
}
/****************************************************************************/

/*
      debug_dsp(cc)
      M_APM cc;
      {
static char buffer[8192];

m_apm_to_string(buffer, -1, cc);
printf("(dsp func) = [%s]\n",buffer);

      }
*/

