/* $Id: args.h 1.6 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
 * Copyright (c) 1999-2001 by Lars Düning. All Rights Reserved.
 * This file is free software. For terms of use see the file LICENSE.
 *---------------------------------------------------------------------------
 */

#ifndef __ARGS_H__
#define __ARGS_H__ 1

/* Dynamic string array */

typedef struct _sarray
{
  int     size;   /* Number of used strings */
  int     alloc;  /* Number of allocated string pointers */
  char ** strs;   /* Array of string pointers */
} SArray;

/* Information about one include directory */

typedef struct _inclinfo {
  char * pPath;     /* Pathname of the directory */
  char * pSymbol;   /* Symbol to use in Makefiles, NULL if none */
  int    bSysPath;  /* TRUE for system directories */
} Include;

/* Program arguments */

extern short     bVerbose;   /* TRUE: Verbose action */
extern SArray    aFiles;     /* Source files */
extern SArray    aAvoid;     /* Source files to avoid */
extern SArray    aSelect;    /* Selected include files to print */
extern Include * aIncl;      /* Include directory information */
extern int       iInclSize;  /* Number of valid entries in aIncl */
extern short     bSplitPath; /* Include path handling */
extern SArray    aSrcExt;    /* Source file extensions */
extern SArray    aObjExt;    /* Associated object file extensions */
extern SArray    aObjPat;    /* Associated object file patterns */
extern short   * bGiveSrc;   /* For each entry in aSrcExt: TRUE if the
                              * source name shall appear in the
                              * dependency list, too. */
extern char    * sObjExt;    /* Default object file extension */
extern short     bDefGSrc;   /* For sObjExt: TRUE if the source name
                              * shall appear in the depency list, too
                              */
extern char    * sMake;      /* Name of the Makefile */
extern char    * sDepfile;   /* Name of the dependency file */
extern short     bFlat;      /* TRUE: Depfile in flat format */
extern short     bNoWarnExit; /* TRUE: Don't return RETURN_WARN from program */
extern short     bIgnoreMiss; /* TRUE: Ignore missing includes */

/* Prototypes */

extern void parseargs (int argc, char ** argv);
extern int array_addlist (SArray *, int, const char **);
extern int array_addfile (SArray *, const char *);
extern int add_expfile (SArray *, const char *);

#endif
