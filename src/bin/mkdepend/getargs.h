/* $Id: getargs.h 1.2 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
** Copyright (c) 1995-1998 by Lars Düning. All Rights Reserved.
** This file is free software. For terms of use see the file LICENSE.
**---------------------------------------------------------------------------
*/

#ifndef __GETARGS_H__
#define __GETARGS_H__ 1

/* Description of an option */

typedef struct OptionDesc {
  unsigned char cOption;  /* The option character (may be \0) */
  char    * pOption;  /* The option string (may be NULL) */
  int       iNumber;  /* The associated option number */
  short     bValue;   /* True: option takes a value */
} OptionDesc;

/* Callback function type for option handling */

typedef int (*OptionHandler)(int iOption, const char * pValue);

/* Predefined values for OptionDesc.eNumber */

#define OPTION_UNKNOWN   0  /* Option unknown, or 'end of descriptor array' */
#define OPTION_ARGUMENT  1  /* Not an option, but a real argument */

/* --- Prototypes --- */
extern char * basename (const char *);
extern int getargs(int, char **, OptionDesc[], OptionHandler);

/* --- Variables --- */
extern const char * aPgmName;

#endif /* __GETARGS_H__ */
