
/* 
 *  M_APM  -  mapmhsin.c
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
 *      $Id: mapmhsin.c,v 1.4 2007/12/03 01:54:06 mike Exp $
 *
 *      This file contains the Hyperbolic SIN, COS, & TAN functions.
 *
 *      $Log: mapmhsin.c,v $
 *      Revision 1.4  2007/12/03 01:54:06  mike
 *      Update license
 *
 *      Revision 1.3  2002/11/03 21:29:20  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.2  2000/09/23 19:52:56  mike
 *      change divide call to reciprocal
 *
 *      Revision 1.1  2000/04/03 18:16:26  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
/*
 *      sinh(x) == 0.5 * [ exp(x) - exp(-x) ]
 */
void	m_apm_sinh(M_APM rr, int places, M_APM aa)
{
M_APM	tmp1, tmp2, tmp3;
int     local_precision;

tmp1 = M_get_stack_var();
tmp2 = M_get_stack_var();
tmp3 = M_get_stack_var();

local_precision = places + 4;

m_apm_exp(tmp1, local_precision, aa);
m_apm_reciprocal(tmp2, local_precision, tmp1);
m_apm_subtract(tmp3, tmp1, tmp2);
m_apm_multiply(tmp1, tmp3, MM_0_5);
m_apm_round(rr, places, tmp1);

M_restore_stack(3);
}
/****************************************************************************/
/*
 *      cosh(x) == 0.5 * [ exp(x) + exp(-x) ]
 */
void	m_apm_cosh(M_APM rr, int places, M_APM aa)
{
M_APM	tmp1, tmp2, tmp3;
int     local_precision;

tmp1 = M_get_stack_var();
tmp2 = M_get_stack_var();
tmp3 = M_get_stack_var();

local_precision = places + 4;

m_apm_exp(tmp1, local_precision, aa);
m_apm_reciprocal(tmp2, local_precision, tmp1);
m_apm_add(tmp3, tmp1, tmp2);
m_apm_multiply(tmp1, tmp3, MM_0_5);
m_apm_round(rr, places, tmp1);

M_restore_stack(3);
}
/****************************************************************************/
/*
 *      tanh(x) == [ exp(x) - exp(-x) ]  /  [ exp(x) + exp(-x) ]
 */
void	m_apm_tanh(M_APM rr, int places, M_APM aa)
{
M_APM	tmp1, tmp2, tmp3, tmp4;
int     local_precision;

tmp1 = M_get_stack_var();
tmp2 = M_get_stack_var();
tmp3 = M_get_stack_var();
tmp4 = M_get_stack_var();

local_precision = places + 4;

m_apm_exp(tmp1, local_precision, aa);
m_apm_reciprocal(tmp2, local_precision, tmp1);
m_apm_subtract(tmp3, tmp1, tmp2);
m_apm_add(tmp4, tmp1, tmp2);
m_apm_divide(tmp1, local_precision, tmp3, tmp4);
m_apm_round(rr, places, tmp1);

M_restore_stack(4);
}
/****************************************************************************/
