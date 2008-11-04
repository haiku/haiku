
/* 
 *  M_APM  -  mapm_flr.c
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
 *      $Id: mapm_flr.c,v 1.4 2007/12/03 01:39:12 mike Exp $
 *
 *      This file contains the floor and ceil functions
 *
 *      $Log: mapm_flr.c,v $
 *      Revision 1.4  2007/12/03 01:39:12  mike
 *      Update license
 *
 *      Revision 1.3  2002/11/05 23:25:30  mike
 *      use new set_to_zero function instead of copy
 *
 *      Revision 1.2  2002/11/03 21:47:43  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.1  2001/03/25 20:53:29  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/*
 *      input    floor    ceil
 *	-----	------	 ------
 *      329.0    329.0    329.0
 *     -837.0   -837.0   -837.0
 *	372.64   372.0    373.0
 *     -237.52  -238.0   -237.0
 */

/****************************************************************************/
/* 
 *      return the nearest integer <= input
 */
void	m_apm_floor(M_APM bb, M_APM aa)
{
M_APM	mtmp;

m_apm_copy(bb, aa);

if (m_apm_is_integer(bb))          /* if integer, we're done */
  return;

if (bb->m_apm_exponent <= 0)       /* if |bb| < 1, result is -1 or 0 */
  {
   if (bb->m_apm_sign < 0)
     m_apm_negate(bb, MM_One);
   else
     M_set_to_zero(bb);

   return;
  }

if (bb->m_apm_sign < 0)
  {
   mtmp = M_get_stack_var();
   m_apm_negate(mtmp, bb);

   mtmp->m_apm_datalength = mtmp->m_apm_exponent;
   M_apm_normalize(mtmp);

   m_apm_add(bb, mtmp, MM_One);
   bb->m_apm_sign = -1;
   M_restore_stack(1);
  }
else
  {
   bb->m_apm_datalength = bb->m_apm_exponent;
   M_apm_normalize(bb);
  }
}
/****************************************************************************/
/* 
 *      return the nearest integer >= input
 */
void	m_apm_ceil(M_APM bb, M_APM aa)
{
M_APM	mtmp;

m_apm_copy(bb, aa);

if (m_apm_is_integer(bb))          /* if integer, we're done */
  return;

if (bb->m_apm_exponent <= 0)       /* if |bb| < 1, result is 0 or 1 */
  {
   if (bb->m_apm_sign < 0)
     M_set_to_zero(bb);
   else
     m_apm_copy(bb, MM_One);

   return;
  }

if (bb->m_apm_sign < 0)
  {
   bb->m_apm_datalength = bb->m_apm_exponent;
   M_apm_normalize(bb);
  }
else
  {
   mtmp = M_get_stack_var();
   m_apm_copy(mtmp, bb);

   mtmp->m_apm_datalength = mtmp->m_apm_exponent;
   M_apm_normalize(mtmp);

   m_apm_add(bb, mtmp, MM_One);
   M_restore_stack(1);
  }
}
/****************************************************************************/
