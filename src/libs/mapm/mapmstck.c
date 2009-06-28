
/* 
 *  M_APM  -  mapmstck.c
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
 *      $Id: mapmstck.c,v 1.11 2007/12/03 01:58:05 mike Exp $
 *
 *      This file contains the stack implementation for using 
 *	local M_APM variables.
 *
 *      $Log: mapmstck.c,v $
 *      Revision 1.11  2007/12/03 01:58:05  mike
 *      Update license
 *
 *      Revision 1.10  2003/07/21 20:39:38  mike
 *      Modify error messages to be in a consistent format.
 *
 *      Revision 1.9  2003/03/31 21:49:08  mike
 *      call generic error handling function
 *
 *      Revision 1.8  2002/11/03 22:42:05  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.7  2002/05/17 22:05:00  mike
 *      the stack is now dynamically allocated and will grow
 *      at run-time if needed
 *
 *      Revision 1.6  2001/07/16 19:47:04  mike
 *      add function M_free_all_stck
 *
 *      Revision 1.5  2000/09/23 19:27:52  mike
 *      increase stack size for new functionality
 *
 *      Revision 1.4  1999/07/09 00:04:47  mike
 *      tweak stack again
 *
 *      Revision 1.3  1999/07/09 00:02:24  mike
 *      increase stack size for new functions
 *
 *      Revision 1.2  1999/06/20 21:13:18  mike
 *      comment out printf debug and set max stack depth
 *
 *      Revision 1.1  1999/06/19 20:32:43  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

static	int	M_stack_ptr  = -1;
static	int	M_last_init  = -1;
static	int	M_stack_size = 0;

static  char    *M_stack_err_msg = "\'M_get_stack_var\', Out of memory";

static	M_APM	*M_stack_array;

/****************************************************************************/
void	M_free_all_stck()
{
int	k;

if (M_last_init >= 0)
  {
   for (k=0; k <= M_last_init; k++)
     m_apm_free(M_stack_array[k]);

   M_stack_ptr  = -1;
   M_last_init  = -1;
   M_stack_size = 0;

   MAPM_FREE(M_stack_array);
  }
}
/****************************************************************************/
M_APM	M_get_stack_var()
{
void    *vp;

if (++M_stack_ptr > M_last_init)
  {
   if (M_stack_size == 0)
     {
      M_stack_size = 18;
      if ((vp = MAPM_MALLOC(M_stack_size * sizeof(M_APM))) == NULL)
        {
         /* fatal, this does not return */

         M_apm_log_error_msg(M_APM_FATAL, M_stack_err_msg);
        }

      M_stack_array = (M_APM *)vp;
     }

   if ((M_last_init + 4) >= M_stack_size)
     {
      M_stack_size += 12;
      if ((vp = MAPM_REALLOC(M_stack_array, 
      			    (M_stack_size * sizeof(M_APM)))) == NULL)
        {
         /* fatal, this does not return */

         M_apm_log_error_msg(M_APM_FATAL, M_stack_err_msg);
        }

      M_stack_array = (M_APM *)vp;
     }

   M_stack_array[M_stack_ptr]     = m_apm_init();
   M_stack_array[M_stack_ptr + 1] = m_apm_init();
   M_stack_array[M_stack_ptr + 2] = m_apm_init();
   M_stack_array[M_stack_ptr + 3] = m_apm_init();

   M_last_init = M_stack_ptr + 3;

   /* printf("M_last_init = %d \n",M_last_init); */
  }

return(M_stack_array[M_stack_ptr]);
}
/****************************************************************************/
void	M_restore_stack(int count)
{
M_stack_ptr -= count;
}
/****************************************************************************/

