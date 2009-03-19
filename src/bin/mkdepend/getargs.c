/* $Id: getargs.c 1.5 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
 * Commandline argument parser.
 *
 * Copyright (c) 1998-2000 by Lars Düning. All Rights Reserved.
 * This file is free software. For terms of use see the file LICENSE.
 *---------------------------------------------------------------------------
 * This code parses the arguments passed to the program in the count <argc>
 * and the array of strings <argv>. The parser distinguishes options, which
 * start with a '-', from normal arguments; options are further distinguished
 * by their name and may take an additional value. The parser neither
 * imposes nor expects any order of options and arguments.
 *
 * Options are recognized in two forms. In the short form the option must
 * be given as a single '-' followed by a single letter. In the long form,
 * options start with '--' followed by a string of arbitrary length.
 * Short options are case sensitive, long options aren't.
 * Examples are: '-r' and '--recursive'.
 *
 * If an option takes a value, it must follow the option immediately after
 * a separating space or '='. Additionally, the value for a short option
 * may follow the option without separator. Examples are: '-fMakefile',
 * '-f Makefile', '--file=Makefile' and '--file Makefile'.
 *
 * Short options may be collated into one argument, e.g. '-rtl', but
 * of these only the last may take a value.
 *
 * The option '--' marks the end of options. All following command arguments
 * are considered proper arguments even if they start with a '-' or '--'.
 *-------------------------------------------------------------------------
 * Internally every option recognized by the program is associated with
 * an id number, assigned by the user. The parser itself uses the two
 * id numbers 'OPTION_UNKNOWN' for unrecognized options, and 
 * 'OPTION_ARGUMENT' for proper command arguments.
 *
 * Id numbers are associated with their option strings/letters by the
 * the array aOptions, which is passed to the parser function. Every element
 * in this array is a structure defining the option's name (string
 * or letter), the associated id number, and whether or not the option
 * takes a value. An element can hold just one type of option (short or
 * long) by setting the description the other option to \0 resp. NULL.
 * The order of the elements does not matter. The end of the array is
 * marked with an element using OPTION_UNKNOWN as assigned id number.
 *
 * The parsing is done by calling the function
 *
 *   int getargs( int argc, char ** argv
 *              , const OptionDesc aOptions[]
 *              , OptionHandler handler)
 *
 * The function is passed the argument count <argc> and vector <argv> as
 * they were received from the main() function, the option description
 * array <aOptions> and a callback function* <handler>. getargs()
 * returns 0 if the parsing completed successfully, and non-zero else.
 *
 * The handler function is called for every successfully recognized option
 * and argument. Its prototype is
 *
 *   int handler(int iOption, const char *pValue)
 *
 * Parameter <iOption> denotes the recognized option, and pValue points
 * to the beginning of the value string if the option takes a value.
 * Proper arguments are parsed with eOption==OPTION_ARGUMENT and pValue
 * pointing to the argument string. The handler has to return 0 if the
 * option/argument was processed correctly, and non-zero else.
 *
 * Additionally, getargs() puts the basename of the program (as determined
 * by argv[0]) into the exported variable
 *
 *    const char *aPgmName
 *
 * when called the first time.
 *-------------------------------------------------------------------------
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getargs.h"

const char *aPgmName = NULL;
  /* The exported program name. */

#define MY_TRUE  1
#define MY_FALSE 0

/*-------------------------------------------------------------------------*/
int
getargs ( int argc, char ** argv
        , OptionDesc aOptions[]
        , OptionHandler opt_eval
        )

/* Get the arguments from the commandline and pass them 
 * as (number, optional value) to the <opt_eval> callback.
 * The options are defined by the array <aOptions>, with an OPTION_UNKNOWN
 * element marking the last entry.
 *
 * If opt_eval() returns non-zero, argument scanning is terminated.
 * In that case, or if getargs() detects an error itself, getargs() returns
 * non-zero.
 * A zero return means 'success' in both cases.
 *
 * Additionally, on the first call getargs() sets aPgmName to the basename
 * of the program as derived from argv[0].
 */

{
  int         i;        /* all purpose */
  int         nOptions; /* Number of options */
  int         iArg;     /* Number of argument under inspection */
  int         eOption;  /* The current recognized option */
  int         xOption;  /* The index of the recog otion in aOptions */
  short       bCont;    /* True: find another option in the same argument */
  short       bDone;    /* True: all options parsed, only args left */
  short       bShort;   /* True: current argument is a short option */
  char      * pArg;     /* Next argument character to consider */
  
  assert(aOptions != NULL);
  assert(opt_eval != NULL);

  /* Determine the program's name */
  if (NULL == aPgmName)
  {
    char * newName;

    newName = strdup(basename(argv[0]));
    if (NULL == newName)
      aPgmName = (const char *)argv[0];
    else
      aPgmName = (const char *)newName;
  }

  /* Make the compiler happy */
  bShort = MY_FALSE;
  pArg = NULL;

  /* Count the options */
  for (nOptions = 0; aOptions[nOptions].iNumber != OPTION_UNKNOWN; nOptions++)
    /* SKIP */ ;

  /* Scan all arguments */
  bCont = MY_FALSE;
  bDone = MY_FALSE;
  for (iArg = 1; iArg < argc; !bCont ? iArg++ : iArg)
  {
    ssize_t   iArglen;     /* Length of remaining argument */
    char   * pValue;       /* First character of an option value, or NULL */
    int      bTakesValue;  /* This option takes a value */

    /* Make the compiler happy */
    iArglen = 0;
    pValue = NULL;
    bTakesValue = MY_FALSE;

    if (bDone)
      eOption = OPTION_ARGUMENT;
    else
    /* If this is not a continuation, reinitialise the inspection vars.
     * For --opt=val arguments, pValue is set to the first character of val.
     */
    if (!bCont)
    {
      pArg = argv[iArg];
      if ('-' == pArg[0] && '-' == pArg[1]) /* Long option? */
      {
        eOption = OPTION_UNKNOWN;
        bShort = MY_FALSE;
        pArg += 2;
        /* Special case: if the argument is just '--', it marks the
         * end of all options.
         * We set a flag and continue with the next argument.
         */
        if ('\0' == *pArg)
        {
          bDone = MY_TRUE;
          continue;
        }
        pValue = strchr(pArg, '=');
        if (pValue != NULL)
        {
          iArglen = pValue - pArg;
          pValue++;
        }
        else
          iArglen = (signed)strlen(pArg);
      }
      else if ('-' == pArg[0]) /* Short option? */
      {
        eOption = OPTION_UNKNOWN;
        bShort = MY_TRUE;
        pArg++;
        iArglen = (signed)strlen(pArg);
        pValue = NULL;
      }
      else /* No option */
      {
        eOption = OPTION_ARGUMENT;
        pValue = pArg;
        iArglen = 0;
      }
    }
    else
      eOption = OPTION_UNKNOWN;

    /* If the option is not determined yet, do it.
     * Set pValue to the first character of the value if any.
     */
    if (OPTION_UNKNOWN == eOption)
    {
      xOption = 0;
      if (bShort) /* search the short option */
      {
        for (i = 0; i < nOptions; i++)
          if ('\0' != aOptions[i].cOption && *pArg == aOptions[i].cOption)
          {
            xOption = i;
            eOption = aOptions[i].iNumber;
            bTakesValue = aOptions[i].bValue;
            pArg++; iArglen--;  /* Consume this character */
            break;
          }

        /* Consume a following '=' if appropriate */
        if (OPTION_UNKNOWN != eOption 
         && bTakesValue 
         && iArglen > 0 && '=' == *pArg)
        {
          pArg++; iArglen--;
        }

        /* If there is a value following in the same argument, set pValue to
         * it and mark the remaining characters as 'consumed'
         */
        if (OPTION_UNKNOWN != eOption && bTakesValue && iArglen > 0)
        {
          pValue = pArg;
          pArg += iArglen;
          iArglen = 0;
        }
      }
      else  /* search the long option */
      { 
        for (i = 0; i < nOptions; i++)
          if (NULL != aOptions[i].pOption 
           && (size_t)iArglen == strlen(aOptions[i].pOption)
           && !strncasecmp(pArg, aOptions[i].pOption, (unsigned)iArglen))
          {
            xOption = i;
            eOption = aOptions[i].iNumber;
            bTakesValue = aOptions[i].bValue;
            break;
          }
      }

      if (OPTION_UNKNOWN == eOption)
      {
        fprintf(stderr, "%s: Unknown option '", aPgmName);
        if (bShort)
          fprintf(stderr, "-%c", *pArg);
        else
          fprintf(stderr, "--%*.*s", (int)iArglen, (int)iArglen, pArg);
        fputs("'.\n", stderr);
        return 1;
      }
      
      /* If at this point bTakesValue is true, but pValue is still NULL,
       * then the value is in the next argument. Get it if it's there.
       */
      if (bTakesValue && pValue == NULL && iArg + 1 < argc)
      {
        iArg++;
        pValue = argv[iArg];
      }

      /* Signal an error if pValue is still NULL or if it's empty. */
      if (bTakesValue && (pValue == NULL || !strlen(pValue)))
      {
        fprintf(stderr, "%s: Option '", aPgmName);
        if (bShort)
          putc(aOptions[xOption].cOption, stderr);
        else
          fputs(aOptions[xOption].pOption, stderr);
        fputs("' expects a value.\n", stderr);
        return 1;
      }

    } /* if (unknown option) */

    /* Before evaluation of the parsed option, determine 'bCont' */
    bCont = bShort && (iArglen > 0) && !bTakesValue;

    /* --- The option evaluation --- */

    i = (*opt_eval)(eOption, pValue);
    if (i)
      return i;

  } /* for (iArg) */

  return 0;
} /* getargs() */

/*-------------------------------------------------------------------------*/
char *
basename (const char *pName)

/* Find the basename (filename) in a UNIX-style pathname and return
 * a pointer to its first character.
 * Example: basename("/boot/home/data/l.txt") returns a pointer to "l.txt".
 */

{
  char * pBase, * cp;

  pBase = (char *)pName;
  for (cp = (char *)pName; *cp; cp++)
    if ('/' == *cp)
      pBase = cp+1;
  return pBase;
}

/***************************************************************************/
