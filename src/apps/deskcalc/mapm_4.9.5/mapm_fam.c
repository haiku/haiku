
/* 
 *  M_APM  -  mapm_fam.c
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
 *      $Id: mapm_fam.c,v 1.5 2007/12/03 01:36:38 mike Exp $
 *
 *      This file contains the free all memory and similiar functions.
 *
 *      $Log: mapm_fam.c,v $
 *      Revision 1.5  2007/12/03 01:36:38  mike
 *      Update license
 *
 *      Revision 1.4  2003/03/30 21:13:05  mike
 *      remove _log from free list
 *
 *      Revision 1.3  2002/02/14 13:36:27  mike
 *      add conditional compile around free all FFT
 *
 *      Revision 1.2  2001/07/16 18:52:40  mike
 *      add some comments
 *
 *      Revision 1.1  2001/07/16 18:41:44  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

/****************************************************************************/
void	m_apm_free_all_mem()
{
M_free_all_add();    /* call each module which has statically declared data */
M_free_all_div();
M_free_all_exp();

#ifndef NO_FFT_MULTIPLY
M_free_all_fft();
#endif

M_free_all_pow();
M_free_all_rnd();
M_free_all_set();
M_free_all_cnst();
M_free_all_fmul();
M_free_all_stck();
M_free_all_util();
}
/****************************************************************************/
void	m_apm_trim_mem_usage()
{
m_apm_free_all_mem();
m_apm_free(m_apm_init());
}
/****************************************************************************/
