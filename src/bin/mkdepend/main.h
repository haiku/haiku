/* $Id: main.h 1.4 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
** Copyright (c) 1995-2000 by Lars Düning. All Rights Reserved.
** This file is free software. For terms of use see the file LICENSE.
**---------------------------------------------------------------------------
*/

#ifndef __MAIN_H__
#define __MAIN_H__ 1

/* Return codes */

#define RETURN_OK     0  /* Everything is ok */
#define RETURN_WARN   1  /* A warning was issued */
#define RETURN_ERROR  2  /* An error occured */
#define RETURN_FAIL   3  /* A fatal error occured */

/* Boolean values */

#define FALSE 0
#define TRUE 1

/* Macros */

#define IS_SELECT_MODE (aSelect.size != 0)
  /* The macro returns TRUE if only selected dependencies are to be
   * generated.
   */

/* Prototypes */

extern void set_rc (int, int);
extern void exit_doserr (int);
extern void exit_nomem (char *);

#endif
