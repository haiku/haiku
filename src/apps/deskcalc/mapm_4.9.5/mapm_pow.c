
/* 
 *  M_APM  -  mapm_pow.c
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
 *      $Id: mapm_pow.c,v 1.10 2007/12/03 01:46:07 mike Exp $
 *
 *      This file contains the POW function.
 *
 *      $Log: mapm_pow.c,v $
 *      Revision 1.10  2007/12/03 01:46:07  mike
 *      Update license
 *
 *      Revision 1.9  2002/11/05 23:39:42  mike
 *      use new set_to_zero call
 *
 *      Revision 1.8  2002/11/03 22:20:59  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.7  2001/07/16 19:24:26  mike
 *      add function M_free_all_pow
 *
 *      Revision 1.6  2000/09/05 22:15:03  mike
 *      minor tweak
 *
 *      Revision 1.5  2000/08/22 21:22:29  mike
 *      if parameter yy is an integer, call the more
 *      efficient _integer_pow function
 *
 *      Revision 1.4  2000/08/22 20:42:08  mike
 *      compute more digits in the log calculation
 *
 *      Revision 1.3  2000/05/24 20:08:21  mike
 *      update some comments
 *
 *      Revision 1.2  2000/05/23 23:20:11  mike
 *      return 1 when input is 0^0. 
 *
 *      Revision 1.1  2000/05/18 22:10:43  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

static	M_APM   M_last_xx_input;
static	M_APM   M_last_xx_log;
static	int     M_last_log_digits;
static	int     M_size_flag = 0;

/****************************************************************************/
void	M_free_all_pow()
{
if (M_size_flag != 0)
  {
   m_apm_free(M_last_xx_input);
   m_apm_free(M_last_xx_log);
   M_size_flag = 0;
  }
}
/****************************************************************************/
/*
	Calculate the POW function by calling EXP :

                  Y      A                 
                 X   =  e    where A = Y * log(X)
*/
void	m_apm_pow(M_APM rr, int places, M_APM xx, M_APM yy)
{
int	iflag, pflag;
char    sbuf[64];
M_APM   tmp8, tmp9;

/* if yy == 0, return 1 */

if (yy->m_apm_sign == 0)
  {
   m_apm_copy(rr, MM_One);
   return;
  }

/* if xx == 0, return 0 */

if (xx->m_apm_sign == 0)
  {
   M_set_to_zero(rr);
   return;
  }

if (M_size_flag == 0)       /* init locals on first call */
  {
   M_size_flag       = M_get_sizeof_int();
   M_last_log_digits = 0;
   M_last_xx_input   = m_apm_init();
   M_last_xx_log     = m_apm_init();
  }

/*
 *  if 'yy' is a small enough integer, call the more
 *  efficient _integer_pow function.
 */

if (m_apm_is_integer(yy))
  {
   iflag = FALSE;

   if (M_size_flag == 2)            /* 16 bit compilers */
     {
      if (yy->m_apm_exponent <= 4)
        iflag = TRUE;
     }
   else                             /* >= 32 bit compilers */
     {
      if (yy->m_apm_exponent <= 7)
        iflag = TRUE;
     }

   if (iflag)
     {
      m_apm_to_integer_string(sbuf, yy);
      m_apm_integer_pow(rr, places, xx, atoi(sbuf));
      return;
     }
  }

tmp8 = M_get_stack_var();
tmp9 = M_get_stack_var();

/*
 *    If parameter 'X' is the same this call as it 
 *    was the previous call, re-use the saved log 
 *    calculation from last time.
 */

pflag = FALSE;

if (M_last_log_digits >= places)
  {
   if (m_apm_compare(xx, M_last_xx_input) == 0)
     pflag = TRUE;
  }

if (pflag)
  {
   m_apm_round(tmp9, (places + 8), M_last_xx_log);
  }
else
  {
   m_apm_log(tmp9, (places + 8), xx);

   M_last_log_digits = places + 2;

   /* save the 'X' input value and the log calculation */

   m_apm_copy(M_last_xx_input, xx);
   m_apm_copy(M_last_xx_log, tmp9);
  }

m_apm_multiply(tmp8, tmp9, yy);
m_apm_exp(rr, places, tmp8);
M_restore_stack(2);                    /* restore the 2 locals we used here */
}
/****************************************************************************/
