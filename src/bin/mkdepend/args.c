/* $Id: args.c 1.9 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
** Arguments parsing and variables.
**
** Copyright (c) 1996-2001 by Lars Düning. All Rights Reserved.
** This file is free software. For terms of use see the file LICENSE.
**---------------------------------------------------------------------------
** Parsed arguments:
**
**   {-i|--include <includepath>[::<symbol>]}
**   [-i-]
**   {-x|--except  <filepattern>}
**   {-S|--select  <filepattern>}
**   {-s|--suffix  <src_ext>{,<src_ext>}[+][:<obj_ext>] | [+]:<obj_ext>}
**   {-p|--objpat  <src_ext>{,<src_ext>}[+]:<obj_pattern>
**   [-f|--file    <makefile>]
**   [-d|--dep     <depfile>]
**   [-l|--flat]
**   [-m|--ignore-missing]
**   [-w|--no-warn-exit]
**   [-v|--verbose]
**   [-h|-?|--help|--longhelp]
**   [-V|--version]
**   {<filename>}
**
**---------------------------------------------------------------------------
*/

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "getargs.h"
#include "main.h"
#include "glob.h"
#include "version.h"

/*-------------------------------------------------------------------------*/

/* Program arguments */

short     bVerbose  = FALSE;          /* TRUE: Verbose action */
SArray    aFiles    = { 0, 0, NULL }; /* Source files */
SArray    aAvoid    = { 0, 0, NULL }; /* Source files to avoid */
SArray    aSelect   = { 0, 0, NULL }; /* Selected include files to print */
Include * aIncl     = NULL;           /* Include directories */
int       iInclSize = 0;              /* Number of valid entries in aIncl */
static int allocIncl = 0;             /* Allcoated size of pIncl */
short     bSplitPath = FALSE;         /* TRUE: distinguish between normal
                                       * and system include path. */
SArray    aSrcExt   = { 0, 0, NULL }; /* Source file extensions */
static int allocGiveSrc = 0, sizeGiveSrc = 0;  
                                      /* Allocated/used size of *bGiveSrc */
short   * bGiveSrc  = NULL;           /* For each entry in aSrcExt: 
                                       * TRUE if the source name shall appear
                                       * in the dependency list, too. */
SArray    aObjExt   = { 0, 0, NULL }; /* Associated object file extensions */
SArray    aObjPat   = { 0, 0, NULL }; /* Associated object file patterns */
char    * sObjExt   = NULL;           /* Default object file extension */
short     bDefGSrc  = FALSE;          /* For sObjExt: TRUE if the source
                                       * name shall appear in the depency
                                       * list, too */
char    * sMake     = NULL;           /* Name of the Makefile */
char    * sDepfile  = NULL;           /* Name of the dependency file */
short     bFlat     = FALSE;          /* TRUE: Depfile in flat format */
short     bNoWarnExit = FALSE;        /* TRUE: Don't return RETURN_WARN
                                       * from program */
short     bIgnoreMiss = FALSE;        /* TRUE: Ignore missing includes */

/*-------------------------------------------------------------------------*/

/* Every recognized option has a ordinal number */

typedef enum OptNumber {
   cUnknown = OPTION_UNKNOWN    /* unknown option */
 , cArgument = OPTION_ARGUMENT  /* normal argument (for us: filename) */
 , cInclude       /* --include */
 , cExcept        /* --except */
 , cSelect        /* --select */
 , cSuffix        /* --suffix */
 , cObjPat        /* --objpat */
 , cFile          /* --file */
 , cDep           /* --dep */
 , cFlat          /* --flat */
 , cNoWarn        // --no-warn-exit
 , cIgnMiss       // --ignore-missing
 , cHelp          /* --help */
 , cLongHelp      /* --longhelp */
 , cVerbose       /* --verbose */
 , cVersion       /* --version */
} OptNumber;

/* Comprehensive lists of recognized options */

static OptionDesc aOptions[]
  = { { 'i',  "include",  cInclude,  TRUE }
    , { 'I',  NULL,       cInclude,  TRUE }
    , { 'x',  "except",   cExcept,   TRUE }
    , { 'S',  "select",   cSelect,   TRUE }
    , { 's',  "suffix",   cSuffix,   TRUE }
    , { 'p',  "objpat",   cObjPat,   TRUE }
    , { 'f',  "file",     cFile,     TRUE }
    , { 'd',  "dep",      cDep,      TRUE }
    , { 'l',  "flat",     cFlat,     FALSE }
    , { 'm',  "ignore-missing", cIgnMiss, FALSE }
    , { 'w',  "no-warn-exit",   cNoWarn,  FALSE }
    , { 'h',  "help",     cHelp,     FALSE }
    , { '?',  NULL,       cHelp,     FALSE }
    , { '\0', "longhelp", cLongHelp, FALSE }
    , { 'v',  "verbose",  cVerbose,  FALSE }
    , { 'V',  "version",  cVersion,  FALSE }
    };

/*-------------------------------------------------------------------------*/
static void
version (void)

/* Print the version of the program. */

{
    fputs("MkDepend " VERSION " - Makefile Dependency Generator\n"
          "\nReleased: " RELEASE_DATE
          "\nCompiled: " __DATE__
#ifdef __TIME__
                       " " __TIME__
#endif
          "\n", stdout);
}

/*-------------------------------------------------------------------------*/
static void
shortusage (void)

/* Print the short help information to stdout. */

{
    version();
    printf("\nUsage: %s [options] <filename>...\n", aPgmName);
  fputs(
"\nOptions are:\n"
"\n"
"  -i|--include <includepath>[::<symbol>]\n"
"  -x|--except  <filepattern>\n"
"  -S|--select  <filepattern>\n"
"  -s|--suffix  <src_ext>{,<src_ext>}[+][:<obj_ext>]\n"
"  -s|--suffix  [+]:<obj_ext>\n"
"  -p|--objpat  <src_ext>{,<src_ext>}[+]:<obj_pattern>\n"
"  -f|--file    <makefile>\n"
"  -d|--dep     <depfile>\n"
"  -l|--flat\n"
"  -m|--ignore-missing\n"
"  -w|--no-warn-exit\n"
"  -v|--verbose\n"
"  -h|-?|--help|--longhelp\n"
"  -V|--version\n"
       , stdout);
       
} /* shortusage() */

/*-------------------------------------------------------------------------*/
static void
usage (void)

/* Print the help information to stdout. */

{
    version();
    printf("\nUsage: %s [options] <filename>...\n", aPgmName);
  fputs(
"\nOptions are:\n"
"\n"
"  -i|--include <includepath>[::<symbol>]\n"
"    Add <includepath> to the list of searched directories. If given,\n"
"    <symbol> is used in the generated dependencies to denote\n"
"    the <includepath>.\n"
"    The special notation '-i-' is used to split the search path into\n"
"    a non-system and a common part.\n"
"\n"
"  -x|--except  <filepattern>\n"
"    Except all matching files from the root set.\n"
"\n"
"  -S|--select  <filepattern>\n"
"    Only the dependencies for files including <filepattern> are printed,\n"
"    and in those only the included files matching <filepattern> are listed.\n"
"    Except all matching files from the root set.\n"
"\n"
"  -s|--suffix  <src_ext>{,<src_ext>}[+][:<obj_ext>]\n"
"    Set the recognized source file suffixes (and optionally associated\n"
"    object file suffixes).\n"
"\n"
"  -s|--suffix  [+]:<obj_ext>\n"
"    Set the default object file suffix.\n"
"\n"
"  -p|--objpat  <src_ext>{,<src_ext>}[+]:<obj_pattern>\n"
"    Define a complex source/object file name relation.\n"
"    The <objpattern> recognizes as meta-symbols:\n"
"       %s: the full sourcename (w/o suffix)\n"
"       %[-][<][<number>]p: the path part of the sourcename\n"
"         <number>: skip first <number> directories of the path,\n"
"                   defaults to 0.\n"
"         <       : directories are counted from the end.\n"
"         -       : use, don't skip the counted directories.\n"
"       %n: the base of the sourcename (w/o suffix)\n"
"       %%: the character %\n"
"       %x: the character 'x' for every other character.\n"
"\n"
"  -f|--file    <makefile>\n"
"    Update <makefile> with the found dependencies (default is 'Makefile').\n"
"\n"
"  -d|--dep     <depfile>\n"
"    Write the found dependencies into <depfile>.\n"
"\n"
"  -l|--flat\n"
"    When generating a <depfile> write the dependencies in flat form.\n"
"\n"
"  -m|--ignore-missing\n"
"    If an include file is not found, it is NOT added to the dependencies.\n"
"\n"
"  -w|--no-warn-exit\n"
"    If MkDepend generates a warning, it does not return a non-zero exit\n"
"    code to the caller (useful for use within makefiles).\n"
"\n"
"  -v|--verbose\n"
"    Verbose operation.\n"
"\n"
"  -V|--version\n"
"    Display the version of the program and exit.\n"
"\n"
"  --longhelp\n"
"    Display this help and exit.\n"
"\n"
"  -h|-?|--help\n"
"    Display the short help text and exit.\n"
"\n"
       , stdout);
       
}

/*-------------------------------------------------------------------------*/
static int
givesrc_addflag (short bFlag)

/* Add <bFlag> to *bGiveSrc.
 * Return 0 on success, non-0 on error.
 */

{
  short * pFlags;
  assert(sizeGiveSrc <= allocGiveSrc);
  if (sizeGiveSrc+1 > allocGiveSrc)
  {
    pFlags = (short*)realloc(bGiveSrc, sizeof(short)*(allocGiveSrc+4));
    if (!pFlags)
      return 1;
    bGiveSrc = pFlags;
    allocGiveSrc += 4;
  }
  bGiveSrc[sizeGiveSrc] = bFlag;
  sizeGiveSrc++;
  return 0;
}

/*-------------------------------------------------------------------------*/
static int
include_addinfo (char * pPath, char * pSymbol, int bSystem)

/* Add the given data to to *bIncl.
 * Return 0 on success, non-0 on error.
 */

{
  Include * pNew;

  assert(iInclSize <= allocIncl);
  if (iInclSize+1 > allocIncl)
  {
    pNew = realloc(aIncl, sizeof(Include)*(allocIncl+4));
    if (!pNew)
      return 1;
    aIncl = pNew;
    allocIncl += 4;
  }
  aIncl[iInclSize].pPath    = pPath;
  aIncl[iInclSize].pSymbol  = pSymbol;
  aIncl[iInclSize].bSysPath = bSystem;
  iInclSize++;
  return 0;
}

/*-------------------------------------------------------------------------*/
int
array_addfile (SArray *pArray, const char * pName)

/* Add a copy of <*pName> to the string array <pArray>.
 * Return 0 on success, non-0 on error.
 * If pName is NULL, the array is simply extended by a NULL pointer.
 */

{
  char ** pStrs;
  assert(pArray->size <= pArray->alloc);
  if (pArray->size+1 > pArray->alloc)
  {
    pStrs = (char **)realloc(pArray->strs, sizeof(char *)*(pArray->alloc+4));
    if (!pStrs)
      return 1;
    pArray->strs = pStrs;
    pArray->alloc += 4;
  }
  else
    pStrs = pArray->strs;
  if (pName)
  {
    pStrs[pArray->size] = strdup(pName);
    if (!pStrs[pArray->size])
      return 1;
  }
  else
    pStrs[pArray->size] = NULL;
  pArray->size++;
  return 0;
}

/*-------------------------------------------------------------------------*/
int
array_addlist (SArray *pArray, int count, const char ** pList)

/* Add the <count> strings from <pList> to string array <pArray>.
 * pList and contained strings may be freed after return.
 * Return 0 on success, non-0 else.
 */

{
  char ** pStrs;
  int     i;

  assert(pList);
  if (!count)
    return 0;
  assert(pArray->size <= pArray->alloc);
  if (pArray->size+count > pArray->alloc)
  {
    pStrs = (char **)realloc(pArray->strs, sizeof(char *)*(pArray->alloc+count));
    if (!pStrs)
      return 1;
    pArray->strs = pStrs;
    pArray->alloc += count;
  }
  else
    pStrs = pArray->strs;
  for (i = 0; i < count; i++)
  {
    char * pString;
    
    pString = strdup(pList[i]);
    if (!pString)
      return 1;
    pStrs[pArray->size++] = pString;
  }
  return 0;
}

/*-------------------------------------------------------------------------*/
int
add_expfile (SArray *pArray, const char * pName)

/* Glob filename <*pName> and add the filenames to string array <pArray>.
 * Return 0 on success, non-0 else.
 */

{
  glob_t  aGlob;
  int     rc;

  memset(&aGlob, 0, sizeof(aGlob));

  switch (glob(pName, GLOB_NOSORT|GLOB_BRACE|GLOB_TILDE, NULL, &aGlob))
  {
  case GLOB_NOSPACE:
    globfree(&aGlob);
    exit_nomem(" (add_expfile: glob)");
    break;

  case GLOB_ABORTED:
    printf("%s: Error expanding %s\n", aPgmName, pName);
    globfree(&aGlob);
    set_rc(RETURN_ERROR, TRUE);
    break;

  case GLOB_NOMATCH:
    globfree(&aGlob);
    return 0;

  case 0:
    /* No error */
    break;

  default:
    globfree(&aGlob);
    exit_doserr(RETURN_ERROR);
    break;
  }

  assert(aGlob.gl_pathc > 0);

  rc = 0;
  if (aGlob.gl_pathc > 0)
    rc = array_addlist(pArray, aGlob.gl_pathc, (const char **)aGlob.gl_pathv);

  globfree(&aGlob);
  return rc;
}

/*-------------------------------------------------------------------------*/
static int
handler (int eOption, const char * pValue)

/* Callback function for getargs(). It is called when option <eOption>
 * was recognized, possibly also passing an associated value in <pValue>.
 * Return 0 on success, non-zero else.
 */

{
    int i; /* All purpose */

    switch (eOption)
    {
    case cArgument:
      i = array_addfile(&aFiles, pValue);
      if (i)
        exit_nomem(" (args: add file)");
      break;

    case cInclude:
      {
        char * pSym, * pMark;
        char * pVal;

        /* Special case: check for the '-' marker */
        if (!strcmp(pValue, "-"))
        {
          bSplitPath = TRUE;
          break;
        }
        
        /* Since we're going to hack into pValue, possibly extending it
         * by one character, we better make a copy of it. We need it
         * anyway to store the data in aIncl[].
         */
        pVal = malloc(strlen(pValue)+2);
        if (!pVal)
          exit_nomem(" (args: dup include arg)");
        strcpy(pVal, pValue);
        
        /* Allow for <path>::<symbol> notation */
        pSym = NULL;
        pMark = pVal+strlen(pVal);
        if (pMark != pVal)
        {
          pMark--;
          while (!pSym && pMark >= pVal+1)
          {
            if (':' != *pMark)
            {
              pMark--;
              continue;
            }
            if (':' != *(pMark-1))
            {
              pMark -= 2;
              continue;
            }
            pSym = pMark+1;
            *(pMark-1) = '\0';
          }
        }
        if (pSym && !strlen(pSym))
            pSym = NULL;
        else if (pSym)
        {
            pSym = strdup(pSym);
            if (!pSym)
                exit_nomem(" (args: dup include sym)");
        }

        /* Make sure <path> ends in a ':' or '/' */
        pMark = pVal+strlen(pVal);
        if (pMark > pVal && ':' != *(pMark-1) && '/' != *(pMark-1))
        {
          *pMark = '/';
          *(pMark+1) = '\0';
        }
        i = include_addinfo(pVal, pSym, bSplitPath);
        if (i)
            exit_nomem(" (args: add include info)");
      }
      break;

    case cExcept:
      i = add_expfile(&aAvoid, pValue);
      if (i)
          exit_nomem(" (args: add except)");
      break;

    case cSelect:
      i = add_expfile(&aSelect, pValue);
      if (i)
          exit_nomem(" (args: add select)");
      break;

    case cSuffix:
    case cObjPat:
      {
        char * pMark, *pOsfix;
        short  bGotPlus;

        /* Allow for <src_suffix>[+]:<obj_suffix> notation */
        bGotPlus = FALSE;
        pOsfix = NULL;
        pMark = (char *)pValue+strlen(pValue);
        if (pMark != pValue)
        {
          pMark--;
          while (!pOsfix && pMark >= pValue)
          {
            if (':' != *pMark)
            {
              pMark--;
              continue;
            }
            pOsfix = pMark+1;
            if (pMark != pValue && *(pMark-1) == '+')
            {
              bGotPlus = TRUE;
              pMark--;
            }
            *pMark = '\0';
          }
        }
        if (pOsfix && !strlen(pOsfix))
          pOsfix = NULL;

        /* --objpat needs the [+]:<obj_pattern> part! */
        if (cObjPat == eOption && !pOsfix)
        {
          printf("%s: <obj_pattern> not specified.\n", aPgmName);
          set_rc(RETURN_ERROR, TRUE);
        }

        /* [+]:<obj_suffix> alone defines default object suffix */
        if (pOsfix && !strlen(pValue))
        {
          if (sObjExt)
            free(sObjExt);
          sObjExt = strdup(pOsfix);
          if (!sObjExt)
              exit_nomem(" (args: dup obj suffix)");
          bDefGSrc = bGotPlus;
        }
        else
        {
          char * pSfix;

          /* Allow for <sfix1>,<sfix2>,...,<sfixn> for source suffixes */
          for ( pSfix = (char *)pValue
              ; NULL != (pMark = strchr(pSfix, ','))
              ; pSfix = pMark+1
              )
          {
            *pMark = '\0';
            if (strlen(pSfix))
            {
              if (   array_addfile(&aSrcExt, pSfix)
                  || givesrc_addflag(bGotPlus)
                  || (cSuffix == eOption ? array_addfile(&aObjExt, pOsfix)
                                         : array_addfile(&aObjPat, pOsfix)
                     )
                 )
                exit_nomem(" (args: add obj suffix)");
            }
          }
          if (strlen(pSfix))
          {
            if (   array_addfile(&aSrcExt, pSfix)
                || givesrc_addflag(bGotPlus)
                || (cSuffix == eOption ? array_addfile(&aObjExt, pOsfix)
                                       : array_addfile(&aObjPat, pOsfix)
                   )
               )
              exit_nomem(" (args: add last obj suffix)");
          }
        }
      }
      break;

    case cFile:
      if (sMake)
        free(sMake);
      sMake = strdup(pValue);
      if (!sMake)
          exit_nomem(" (args: dup make)");
      break;

    case cDep:
      if (sDepfile)
        free(sDepfile);
      sDepfile = strdup(pValue);
      if (!sDepfile)
          exit_nomem(" (args: dup dep)");
      break;

    case cFlat:
      bFlat = TRUE;
      break;

    case cIgnMiss:
      bIgnoreMiss = TRUE;
      break;

    case cNoWarn:
      bNoWarnExit = TRUE;
      break;

    case cVerbose:
      bVerbose = TRUE;
      break;

    case cHelp:
      shortusage();
      set_rc(RETURN_WARN, TRUE);
      break;

    case cLongHelp:
      usage();
      set_rc(RETURN_WARN, TRUE);
      break;

    case cVersion:
      version();
      set_rc(RETURN_WARN, TRUE);
      break;

    default:
      /* This shouldn't happen. */
      printf("%s: (getargs) Internal error, eOption is %d\n", aPgmName, eOption);
      assert(0);
      break;
    }

    return 0;
}

/*-------------------------------------------------------------------------*/
void
parseargs (int argc, char ** argv)

/* Get the arguments from the commandline and fill in the exported variables.
 * In case of an error, the program is terminated.
 */

{
  if (getargs(argc, argv, aOptions, handler))
      set_rc(RETURN_ERROR, TRUE);
}

/***************************************************************************/
