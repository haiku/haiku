/* $Id: main.c 1.8 Wed, 01 Mar 2000 21:15:40 -0700 lars $ */

/*---------------------------------------------------------------------------
 * Main module of MkDepend.
 *
 * Copyright (c) 1995-2001 by Lars Düning. All Rights Reserved.
 * This file is free software. For terms of use see the file LICENSE.
 *---------------------------------------------------------------------------
 * The module implements the argument parsing and the control flow.
 *
 * Usage:
 *   mkdepend {-i|--include <includepath>[::<symbol>]}
 *            [-i-]
 *            {-x|--except  <filepattern>}
 *            {-S|--select  <filepattern>}
 *            {-s|--suffix  <src_ext>{,<src_ext>}[+][:<obj_ext>] | [+]:<obj_ext>}
 *            {-p|--objpat  <src_ext>{,<src_ext>}[+]:<obj_pattern>
 *            [-f|--file    <makefile>]
 *            [-d|--dep     <depfile>]
 *            [-l|--flat]
 *            [-m|--ignore-missing]
 *            [-w|--no-warn-exit]
 *            [-v|--verbose]
 *            [-h|-?|--help|--longhelp]
 *            [-V|--version]
 *            {<filepattern>}
 *
 * The <objpattern> recognizes as meta-symbols:
 *   %s: the full sourcename (w/o suffix)
 *   %[-][<][<number>]p: the path part of the sourcename
 *     <number>: skip first <number> directories of the path, defaults to 0.
 *     <       : directories are counted from the end.
 *     -       : use, don't skip the counted directories.
 *   %n: the base of the sourcename (w/o suffix)
 *   %%: the character %
 *   %x: the character 'x' for every other character.
 *
 * The '+' in the pattern means that the sourcename itself should also be
 * listed as dependency of the targetname.
 *
 *---------------------------------------------------------------------------
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "args.h"
#include "getargs.h"
#include "main.h"
#include "reader.h"
#include "nodes.h"
#include "util.h"
#include "version.h"

/*-------------------------------------------------------------------------*/
/* Stack structure for the tree output routines */
typedef struct stacknode
 {
   struct stacknode * pNext;  /* Next stacknode */
   struct stacknode * pPrev;  /* Previous stacknode */
   NodeRef * pRef;            /* Next node to work on */
 }
StackNode;
/* Misc Variables */

char * aVersion = "$VER: MkDepend " VERSION " (" __DATE__ ")";
  /* Amiga-style version string */

static struct stat aMakeStat;    /* Stat buffer for the Makefile */
static int         bMakeExists;  /* TRUE: Old Makefile exists */

static int         returncode = RETURN_OK;   /* Global return code */

static StackNode * pStack = NULL;  /* Stackbase for tree output routine */

/*-----------------------------------------------------------------------*/
void
set_rc (int code, int bAbort)

/* Set the global returncode to <code> given that <code> is higher
 * than the current one.
 * If <bAbort>, exit immediately then.
 */

{
  if (code > returncode)
    returncode = code;
  if (bAbort)
    exit(returncode);
}

/*-------------------------------------------------------------------------*/
void
exit_doserr (int code)

/* Print a message according to errno, then exit with <code>.
 */

{
  perror(aPgmName);
  set_rc(code, TRUE);
}

/*-------------------------------------------------------------------------*/
void
exit_nomem (char * msg)

/* Print a "Out of Memory<msg>." message and exit with RETURN_FAIL.
 */

{
  printf("%s: Out of memory%s.\n", aPgmName, msg ? msg : "");
  set_rc(RETURN_FAIL, TRUE);
}

/*-------------------------------------------------------------------------*/
static int
readerror (Node * pNode, int rc, int bFindError)

/* Print an error message after the file for <pNode> couldn't be opened
 * for reading resp. if the file could not be found.
 *
 * Result is the new return code, based on the given <rc>.
 */

{
    size_t slen; /* Linelen so far */
    size_t len;
    int bFlag;  /* TRUE if at least one name has been printed in this line */
    NodeRef * pRef;
    Node    * pRNode;

    if (rc < RETURN_ERROR && (!bFindError || !bIgnoreMiss))
        rc = RETURN_WARN;
    if (!bFindError)
    {
        perror(aPgmName);
        printf("%s: Warning: Can't read '%s'.\n", aPgmName, pNode->pName);
    }
    else if (!bIgnoreMiss)
        printf("%s: Warning: Can't find '%s'.\n", aPgmName, pNode->pName);
    else
        printf("%s: Info: Can't find '%s'.\n", aPgmName, pNode->pName);

    pRef = pNode->pUsers;
    if (pRef)
    {
        printf("%s:   Included at least by:", aPgmName);
        slen = strlen(aPgmName)+25;
        bFlag = FALSE;

        for ( ; pRef ; pRef = pRef->pNext)
        {
            pRNode = pRef->pNode;
            len = strlen(pRNode->pName);
            if (slen + len + 2 > 75)
            {
                if (bFlag)
                    putchar(',');
                putchar('\n');
                slen = 0;
                bFlag = FALSE;
            }
            if (!slen)
            {
                printf("%s:    ", aPgmName);
                slen = strlen(aPgmName)+5;
            }
            if (bFlag)
            {
                putchar(',');
                slen++;
            }
            printf(" %s", pRNode->pName);
            slen += len+1;
            bFlag = TRUE;
        }
        if (slen)
            putchar('\n');
    }

    return rc;
}

/*-------------------------------------------------------------------------*/
static int
readfiles (void)

/* Read and analyse all files.
 * If a file can't be read, print a message.
 * Return 0 if all went well, RETURN_WARN if one of the files could not be
 * found, RETURN_ERROR if one of the files could not be read properly.
 */

{
    int           rc;       /* Return value */
    Node        * pNode;    /* Node of the file under examination */
    int           srcRead;  /* Number of source files read */
    const char  * pName;    /* Includefile name returned by reader */

    rc = 0;
    srcRead = 0;

    while (NULL != (pNode = nodes_todo()) )
    {
        int bSystem; /* TRUE if it's a <> include */

        if (pNode->flags & NODE_NOTFND)
            continue;

        if (!reader_open(pNode->pName))
        {
            rc = readerror(pNode, rc, FALSE);
            continue;
        } /* If (can't open pNode->pName) */

        if (bVerbose)
        {
            printf(" reading %-65s\r", pNode->pName); fflush(stdout);
        }

        while (NULL != (pName = reader_get(&bSystem)) )
        {
            int    index;    /* index into aIncl[] */
            Node * pChild;   /* Node of included file, NULL if not found */
            Node * pChFirst; /* First 'direct' child node */

            for (index = -1, pChild = NULL, pChFirst = NULL
                ; !pChild && index < iInclSize
                ; index++)
            {
                char aName[FILENAME_MAX+1];
                char * pCName;  /* aName[] cleaned up */
                char * pCBase;  /* Namepart in *pCName */

                /* Don't look for system includes in non-system directories. */
                if (index >= 0 && bSplitPath && bSystem && !aIncl[index].bSysPath)
                    continue;

                aName[FILENAME_MAX] = '\0';

                if (index < 0)
                {
                    /* Special case: look directly for the filename.
                     * If bSplitPath is given, or if the filename is absolute,
                     * use it unchanged. Else, use the path of the including
                     * file as anchor.
                     */

                    if (bSplitPath || '/' == *pName)
                        strcpy(aName, pName);
                    else
                    {
                        size_t oFilename;

                        oFilename = (unsigned)(pNode->pBase - pNode->pName);
                        strncpy(aName, pNode->pName, oFilename);
                        strcpy(aName+oFilename, pName);
                    }
                }
                else
                {
                    /* Standard case: build the pathname from the include
                     * pathname.
                     */

                    strcpy(aName, aIncl[index].pPath);
                    strcat(aName, pName);
                }

                /* Cleanup the filename and check if it is listed in
                 * the tree
                 */
                pCName = util_getpath(aName, &pCBase);
                if (!pCName)
                {
                    if (bVerbose)
                        printf("%-78s\r", "");
                    reader_close();
                    exit_nomem(" (readfiles: 1. util_getpath)");
                }

                pChild = nodes_findadd(pCName, TRUE);

                if (pChild->flags & NODE_NEW)
                {
                    /* New file: check if it exists and set up the
                     * node properly
                     */

                    struct stat aStat;

                    pChild->pName = pCName;
                    pChild->pBase = pCBase;

                    if (index >= 0)
                    {
                        pChild->pPath = util_getpath(pName, NULL);
                        pChild->iInclude = index;
                    }
                    else if (pNode->iInclude < 0 || pNode->flags & NODE_SOURCE)
                    {
                        pChild->pPath = util_getpath(pCName, NULL);
                        pChild->iInclude = -1;
                    }
                    else
                    {
                        strcpy(aName, pNode->pPath);
                        strcat(aName, pName);
                        pChild->pPath = util_getpath(aName, NULL);
                        pChild->iInclude = pNode->iInclude;
                    }
                    if (!pChild->pPath)
                    {
                        if (bVerbose)
                            printf("%-78s\r", "");
                        reader_close();
                        exit_nomem(" (readfiles: 2. util_getpath)");
                    }

                    pChild->flags = (short)(bSystem ? NODE_SYSTEM : 0);

                    /* It's ok to not find <>-includes - they just don't have
                     * to appear in the output then.
                     */
                    if (stat(pCName, &aStat))
                        pChild->flags |= NODE_NOTFND | (bSystem ? NODE_IGNORE : 0);
                }

                /* The pChFirst is needed if the include can't be found
                 * so we can keep track from where it is included.
                 */
                if (index < 0)
                    pChFirst = pChild;

                /* Test if the file for pChild exists.
                 */
                if (pChild->flags & NODE_NOTFND)
                {
                    /* Make sure that the file is listed in the warnings
                     * if appropriate.
                     */
                    if (index >= 0)
                        pChild->flags |= NODE_IGNORE;
                    else if ((pChild->flags & NODE_IGNORE) && !bSystem)
                        pChild->flags ^= NODE_IGNORE;
                    pChild = NULL;
                }
                
                /* For absolute pNames, only index -1 is tried */
                if ('/' == *pName)
                    break;
            } /* for (index = -1..iInclSize) */

            assert(pChFirst != NULL);

            if (pChild ? nodes_depend(pChild, pNode) 
                       : nodes_depend(pChFirst, pNode))
            {
                if (bVerbose)
                    printf("%-78s\r", "");
                reader_close();
                exit_nomem(" (readfiles: nodes_depend)");
            }
        } /* while(get pName from file */

        if (bVerbose)
            printf("%-78s\r", "");
        if (!reader_eof() || reader_close())
        {
            perror(aPgmName);
            printf("%s: Error reading '%s'\n", aPgmName, pNode->pName);
            rc = RETURN_ERROR;
        }
        else if (pNode->flags & NODE_SOURCE)
        {
            srcRead++;
        }
    } /* while (nodes_todo()) */

    if (!srcRead)
    {
        printf("%s: No source file read.\n", aPgmName);
        rc = RETURN_ERROR;
    }

    /* Walk tree and print all include files not found */
    nodes_initwalk();
    while (rc != RETURN_ERROR && NULL != (pNode = nodes_inorder()) )
    {
        if ((pNode->flags & (NODE_NOTFND|NODE_IGNORE)) == NODE_NOTFND)
        {
            if (pNode->pUsers)
                rc = readerror(pNode, rc, TRUE);
            else
                pNode->flags |= NODE_IGNORE;
        }
    }

    if (bVerbose)
        fflush(stdout);

    return rc;
}

/*-------------------------------------------------------------------------*/
static void
make_objname ( char *pBuf, const char *pName, int slen
             , const char * pObjExt, const char *pObjPat
             )

/* Construct the name of the dependency target.
 *
 *   pBuf   : Buffer to construct the name in.
 *   pName  : Name of the sourcefile.
 *   slen   : Length of the sourcefile suffix.
 *   pObjExt: Object extensions for this sourcefile, may be NULL.
 *   pObjPat: Object pattern for this sourcefile, may be NULL.
 *
 * If pObjPat is not NULL, the pattern is used to construct the name.
 * If pObjPat is NULL, the pObjExt string is appended to the sourcefile
 * name (minus its source suffix) to construct the name. If pObjExt is
 * NULL, the default object extension is used.
 */

{
  size_t nlen, plen;
  char * pBasename;
  const char * pSrc, * pMark;
  char *pDst, ch;
  short flags;
  long  number;

#define GOT_MINUS   (1<<0)
#define GOT_ANGLE   (1<<1)
#define GOT_NUMBER  (1<<2)
#define NOT_DONE    (1<<3)
#define NO_PATTERN  (1<<4)

  number = 0;
  nlen = strlen(pName)-slen;
  if (!pObjPat)
  {
    strcpy(pBuf, pName);
    if (pObjExt)
      strcpy(pBuf+nlen, pObjExt);
    else
      strcpy(pBuf+nlen, sObjExt);
    return;
  }
  pBasename = basename(pName);
  plen = (unsigned)(pBasename-pName);
  pSrc = pObjPat;
  pDst = pBuf;
  while ('\0' != (ch = *pSrc++))
  {
    if (ch != '%')
    {
      *pDst++ = ch;
      continue;
    }

    pMark = pSrc;
    flags = 0;
    do
    {
      ch = *pSrc++;
      switch(ch)
      {
      case '-':
        if (flags & (GOT_MINUS|GOT_ANGLE|GOT_NUMBER))
          flags = NO_PATTERN;
        else
          flags |= GOT_MINUS|NOT_DONE;
        break;

      case '<':
        if (flags & (GOT_ANGLE|GOT_NUMBER))
          flags = NO_PATTERN;
        else
          flags |= GOT_ANGLE|NOT_DONE;
        break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (flags & GOT_NUMBER)
          flags = NO_PATTERN;
        else
        {
          char * pTail;

          number = strtol(pSrc-1, &pTail, 10);
          pSrc = pTail;
          flags |= GOT_NUMBER|NOT_DONE;
        }
        break;

      case 's':
        strncpy(pDst, pName, nlen);
        pDst += nlen;
        flags = 0;
        break;

      case 'p':
        if (pName != pBasename
        && (!(flags & GOT_MINUS) || (flags & GOT_NUMBER))
        )
        {
          const char * cp;
          size_t cplen;

          cp = pName;
          cplen = plen;
          if (flags & GOT_NUMBER)
          {
            if (flags & GOT_ANGLE)
            {
              cp = pName+plen;
              number++;
              while (number)
              {
                --cp;
                if (*cp == '/' || *cp == ':')
                {
                  number--;
                  if (!number)
                    cp++;
                }
                if (cp == pName)
                  break;
              }
            }
            else
            {
              cp = pName;
              while (number && cp != pBasename)
              {
                if (*cp == '/' || *cp == ':')
                {
                  number--;
                }
                cp++;
              }
            }
            /* cp now points to the character after the
             * determined path part.
             */

            if (flags & GOT_MINUS)
            {
              cplen = (unsigned)(cp - pName);
              cp = pName;
            }
            else
              cplen = plen - (cp-pName);
          }
          if (cplen)
          {
            strncpy(pDst, cp, cplen);
            pDst += cplen;
          }
        }
        flags = 0;
        break;

      case 'n':
        strncpy(pDst, pBasename, nlen-plen);
        pDst += nlen-plen;
        flags = 0;
        break;

      case '%':
        *pDst++ = '%';
        flags = 0;
        break;

      default:
        flags = NO_PATTERN;
        break;
      }
    }
    while (flags & NOT_DONE);
    if (flags & NO_PATTERN)
      pSrc = pMark; /* to be read again as normal text */
  }
  *pDst = '\0';

#undef GOT_MINUS
#undef GOT_ANGLE
#undef GOT_NUMBER
#undef NOT_DONE
#undef NO_PATTERN
}

/*-------------------------------------------------------------------------*/
static int
output_node_tree (Node * pNode, int bUsers, int bSelect)

/* Output the collected dependencies (bUsers is FALSE) resp. the collected
 * users (bUsers is TRUE) in tree form into the Depfile.
 * If <bSelect> is TRUE, only files marked as selected are considered.
 *
 * Return 0 on success, RETURN_WARN on a mild error, RETURN_ERROR on a
 * severe error.
 */

{
  StackNode * pTop;  /* Stackbase, next free Stacktop */
  int         rc;
  Node    * pRNode;  /* referenced node */
  NodeRef * pRef;    /* Next node to work on */
  int       level;   /* Depth in dependency tree */
  char      aObjname[FILENAME_MAX+1];
  int       i, len;

  assert(pNode);

  rc = RETURN_OK;

  /* If bSelect is true, but the pNode is not marked as selected,
   * there is nothing to print.
   */
  if (bSelect && !(pNode->flags & (NODE_SELECT|NODE_ISELECT)))
      return rc;

  do
  {
    pTop = pStack;

    aObjname[FILENAME_MAX] = '\0';

    /* Print name of the basenode */
    if (!bUsers)
    {
      if (reader_write(pNode->pName))
      {
        rc = RETURN_ERROR;
        break; /* outer while */
      }
      if (reader_write((pNode->flags & NODE_NOTFND) ? " : (not found)\n" 
                                                    : " :\n")
         )
      {
        rc = RETURN_ERROR;
        break; /* outer while */
      }
    }

    /* Initialize the tree output */
    pTop->pRef = bUsers ? pNode->pUsers : pNode->pDeps;
    level = 1;
    pRef = pTop->pRef;

    /* Tree output loop
     */
    while (1)
    {
      int bQuoteIt;

      /* On end of this level: pop stack */
      if (!pRef)
      {
        if (pTop == pStack)
          break;
        pRef = pTop->pRef;
        level--;
        pTop = pTop->pPrev;
        continue;
      }

      /* If the file wasn't found, skip this node. */
      if (pRef->pNode->flags & NODE_IGNORE)
      {
        pRef = pRef->pNext;
        continue;
      }

      /* If bSelect is true, but this node is not marked, skip it
       */
      if (bSelect && !(pRef->pNode->flags & (NODE_SELECT|NODE_ISELECT)))
      {
        pRef = pRef->pNext;
        continue;
      }

      /* Output filename of referenced node */
      pRNode = pRef->pNode;
      if (!(pRNode->flags & NODE_SOURCE) && pRNode->iInclude >= 0)
      {
        if (aIncl[pRNode->iInclude].pSymbol)
          strcpy(aObjname, aIncl[pRNode->iInclude].pSymbol);
        else
          strcpy(aObjname, aIncl[pRNode->iInclude].pPath);
        strcat(aObjname, pRNode->pPath);
        strcat(aObjname, pRNode->pBase);
      }
      else
      {
        strcpy(aObjname, pRNode->pName);
      }
      assert(aObjname[FILENAME_MAX] == '\0');
      len = (signed)strlen(aObjname);
      bQuoteIt = (strchr(aObjname+1, ' ') != NULL);
      for (i = 0; i < level-1; i++)
        if (reader_writen("    ", 4))
        {
          rc = RETURN_ERROR;
          break; /* for */
        }
      if (!rc && i < level && reader_writen(bUsers ? " -> " : " <- ", 4))
        rc = RETURN_ERROR;
      if (!rc && bQuoteIt && reader_writen("'", 1))
        rc = RETURN_ERROR;
      if (!rc && reader_writen(aObjname, (size_t)len))
        rc = RETURN_ERROR;
      if (!rc && bQuoteIt && reader_writen("'", 1))
        rc = RETURN_ERROR;
      if (!rc && (pRNode->flags & NODE_NOTFND) && reader_writen(" (not found)", 12))
        rc = RETURN_ERROR;
      if (!rc && reader_writen("\n", 1))
        rc = RETURN_ERROR;
      if (rc)
        break; /* tree-while() */

      /* Push this level onto the stack */
      if (!pTop->pNext)
      {
        StackNode * pNew;
        pNew = malloc(sizeof(*pNew));
        if (!pNew)
        {
          if (bVerbose)
            printf("%-78s\r", "");
          printf("%s: Out of memory.\n", aPgmName);
          rc = RETURN_ERROR;
          break; /* tree-while */
        }
        memset(pNew, 0, sizeof(*pNew));
        pTop->pNext = pNew;
        pNew->pPrev = pTop;
        pTop = pNew;
      }
      else
      {
        pTop = pTop->pNext;
      }
      pTop->pRef = pRef->pNext;

      /* Continue with next deeper level */
      level++;
      pRef = bUsers ? pRNode->pUsers : pRNode->pDeps;
    } /* end while(1) - tree output loop */

  } while(0);

  return rc;
}

/*-------------------------------------------------------------------------*/
static int
output_tree (void)

/* Output the collected dependencies in tree form into the Depfile.
 * Return 0 on success, RETURN_WARN on a mild error, RETURN_ERROR on a
 * severe error.
 */

{
  int         rc;
  char      * pFile;    /* Name of file to create */
  Node      * pNode;

  pFile = sDepfile;
  assert(pFile);

  rc = RETURN_OK;
  do
  {
    if (!pStack)
    {
      /* Initialise stack */
      pStack = malloc(sizeof(*pStack));
      if (!pStack)
      {
        printf("%s: Out of memory\n", aPgmName);
        rc = RETURN_ERROR;
        break;
      }
      memset(pStack, 0, sizeof(*pStack));
    }

    /* Open files */
    if (reader_openrw(NULL, pFile))
    {
      perror(aPgmName);
      printf("%s: Can't write '%s'\n", aPgmName, pFile);
      rc = RETURN_ERROR;
      break;
    }

    if (bVerbose)
    {
      printf(" creating '%s'\r", pFile);
      fflush(stdout);
    }

    /* Walk and output the dependencies and users of the
     * skeleton sources.
     */
    nodes_initwalk();
    while (rc != RETURN_ERROR && NULL != (pNode = nodes_inorder()) )
    {
      if (!(pNode->flags & (NODE_IGNORE)) 
       && (pNode->flags & (NODE_SOURCE|NODE_AVOID)) == NODE_SOURCE
       && (!IS_SELECT_MODE || (pNode->flags & (NODE_SELECT|NODE_ISELECT)))
       && !((pNode->flags & NODE_NOTFND) && bIgnoreMiss)
         )
      {
        rc = output_node_tree(pNode, FALSE, IS_SELECT_MODE);
        if (!rc)
          rc = output_node_tree(pNode, TRUE, IS_SELECT_MODE);
        if (!rc && reader_writen("\n", 1))
          rc = RETURN_ERROR;
      }
    }

    /* Walk and output the dependencies and users of the
     * non-skeleton sources
     */
    nodes_initwalk();
    while (rc != RETURN_ERROR && NULL != (pNode = nodes_inorder()) )
    {
      if (!(pNode->flags & (NODE_IGNORE|NODE_SOURCE))
       && (!IS_SELECT_MODE || (pNode->flags & (NODE_SELECT|NODE_ISELECT)))
       && !((pNode->flags & NODE_NOTFND) && bIgnoreMiss)
         )
      {
        rc = output_node_tree(pNode, FALSE, IS_SELECT_MODE);
        if (!rc)
          rc = output_node_tree(pNode, TRUE, IS_SELECT_MODE);
        if (!rc && reader_writen("\n", 1))
          rc = RETURN_ERROR;
      }
    }

    if (rc == RETURN_ERROR)
    {
      int i = errno;
      if (bVerbose)
        printf("%-78s\r", "");
      errno = i;
      perror(aPgmName);
      printf("%s: Error writing '%s'.\n", aPgmName, pFile);
      rc = RETURN_ERROR;
      break;
    }

    if (reader_writeflush())
    {
      int i = errno;
      if (bVerbose)
        printf("%-78s\r", "");
      errno = i;
      perror(aPgmName);
      printf("%s: Error writing '%s'.\n", aPgmName, sDepfile);
      rc = RETURN_ERROR;
      break;
    }
    /* Finish up */
    if (bVerbose)
      printf("%-78s\r", "");

    if (reader_close())
    {
      perror(aPgmName);
      printf("%s: Error writing '%s'.\n", aPgmName, pFile);
      rc = RETURN_ERROR;
      break;
    }

  } while(0);

  while (pStack)
  {
    StackNode * pTop;

    pTop = pStack;
    pStack = pStack->pNext;
    free(pTop);
  }

  /* Error cleanup */
  if (rc == RETURN_ERROR)
  {
    reader_close();
    remove(pFile);
  }

  if (bVerbose && rc != RETURN_ERROR)
    printf("%-78s\r", "");

  return rc;
}

/*-------------------------------------------------------------------------*/
static int
output_list_flat ( NodeRef * pRef, int startlen
                 , const char * pIndent, int bAsMake)

/* Output the given list of dependencies/users, starting with pRef,
 * with bAsMake interpreted as below. startlen is the line length
 * printed at time of call, pIndent is the indentation string to use.
 *
 * Return 0 on success, RETURN_WARN on a mild error, RETURN_ERROR on a
 * severe error.
 */

{
  int       rc;
  Node    * pRNode;
  size_t    slen;           /* Linelen so far */
  size_t    len;
  size_t    ilen;           /* Length of pIndent */
  char      aObjname[FILENAME_MAX+1];

  assert(pIndent);

  rc = RETURN_OK;
  aObjname[FILENAME_MAX] = '\0';

  slen = (size_t)startlen;
  ilen = strlen(pIndent);

  for ( ; rc != RETURN_ERROR && pRef; pRef = pRef->pNext)
  {
    pRNode = pRef->pNode;

    /* If the file was not found and system include, skip the node */
    if (pRNode->flags & (NODE_IGNORE))
      continue;

    /* If the file was not found and the user wants to ignore missing
     * includes, skip the node.
     */
    if ((pRNode->flags & NODE_NOTFND) && bIgnoreMiss)
      continue;

    /* Construct the name of the dependee */
    aObjname[0] = ' ';
    aObjname[1] = '\0';
    if (pRNode->iInclude >= 0)
    {
      short bQuoteIt;
      char * pIPath;
      
      if (aIncl[pRNode->iInclude].pSymbol)
        pIPath = aIncl[pRNode->iInclude].pSymbol;
      else
        pIPath = aIncl[pRNode->iInclude].pPath;
        
      bQuoteIt = (NULL != strchr(pIPath, ' ')) 
              || (NULL != strchr(pRNode->pPath, ' '))
              || (NULL != strchr(pRNode->pBase, ' '));

      if (bQuoteIt) strcat(aObjname, "\"");
      strcat(aObjname, pIPath);
      strcat(aObjname, pRNode->pPath);
      strcat(aObjname, pRNode->pBase);
      if (bQuoteIt) strcat(aObjname, "\"");
    }
    else
    {
      short bQuoteIt;
      bQuoteIt = (NULL != strchr(pRNode->pName, ' '));

      if (bQuoteIt) strcat(aObjname, "\"");
      strcat(aObjname, pRNode->pName);
      if (bQuoteIt) strcat(aObjname, "\"");
    }
    assert(aObjname[FILENAME_MAX] == '\0');
    len = strlen(aObjname);

    /* Fit it into the line */
    if (slen > 0)
    {
      if (slen+len > 75)
      {
        if (reader_writen(bAsMake ? " \\\n" : "\n", (size_t)(bAsMake ? 3 : 1)))
        {
          rc = RETURN_ERROR;
          break; /* inner for */
        }
        slen = 0;
      }
    }
    if (!slen)
    {
      if (reader_writen(pIndent, ilen))
      {
        rc = RETURN_ERROR;
        break; /* inner for */
      }
      slen = ilen;
    }
    if (reader_writen(aObjname, len))
    {
      rc = RETURN_ERROR;
      break; /* inner for */
    }
    slen += len;
  } /* for () */
  if (slen && reader_writen("\n", 1))
    rc = RETURN_ERROR;

  return rc;
}

/*-------------------------------------------------------------------------*/
static int
output_node_flat (Node * pNode, int bAsMake, int bSelect)

/* For a given node pNode Output the collected dependencies/users into a
 * file, either as Makefile (bAsMake is TRUE) or as Depfile (bAsMake is
 * FALSE).
 * The file must have been opened by the caller in the reader module.
 * If <bSelect> is true, only files marked as SELECTed are printed.
 *
 * Return 0 on success, RETURN_WARN on a mild error, RETURN_ERROR on a
 * severe error.
 */

{
  int       rc;
  NodeRef * pList, * pRef;
  long      suffix;         /* Suffix index */
  size_t    slen;           /* Linelen so far */
  size_t    len;
  char      aObjname[FILENAME_MAX+1];

  assert(pNode);

  rc = RETURN_OK;
  aObjname[FILENAME_MAX] = '\0';

  /* If bSelect is true, but the pNode is not marked as selected,
   * there is nothing to print.
   */
  if (bSelect && !(pNode->flags & (NODE_SELECT|NODE_ISELECT)))
    return rc;

  /* First, output the dependencies list always */
  pList = nodes_deplist(pNode, FALSE, bSelect);
  assert(pList);
  pRef = pList;

  /* Check for a given suffix.
   * Search backwards in case later definitions overwrote
   * earlier ones.
   */
  slen = strlen(pNode->pName);
  len = 0;
  if (!bAsMake)
    suffix = -1;
  else
  for (suffix = aSrcExt.size-1; suffix >= 0; suffix--)
  {
    len = strlen(aSrcExt.strs[suffix]);
    if (!strcmp(pNode->pName+slen-len, aSrcExt.strs[suffix]))
      break;
  }

  do {
    /* Construct the name of the dependency target and write it */
    if (suffix >= 0)
    {
      char * pObjPat, * pObjExt;
      short bQuoteIt;

      pObjExt = aObjExt.size ? aObjExt.strs[suffix] : NULL;
      pObjPat = aObjPat.size ? aObjPat.strs[suffix] : NULL;

      make_objname( aObjname, pNode->pName, (int)len, pObjExt, pObjPat);
      assert(aObjname[FILENAME_MAX] == '\0');
      bQuoteIt = (NULL != strchr(aObjname, ' '));
      
      if ((bQuoteIt && reader_write("\""))
       || reader_write(aObjname)
       || (bQuoteIt && reader_write("\""))
         )
      {
        rc = RETURN_ERROR;
        break; /* while */
      }
      slen = strlen(aObjname);

      if (!((pObjExt || pObjPat) ? bGiveSrc[suffix] : bDefGSrc))
        pRef = pRef->pNext;
    }
    else
    {
      short bQuoteIt;

      bQuoteIt = (NULL != strchr(pNode->pName, ' '));
      if ((bQuoteIt && reader_write("\""))
       || reader_write(pNode->pName)
       || (bQuoteIt && reader_write("\""))
         )
      {
        rc = RETURN_ERROR;
        break; /* outer while */
      }
      pRef = pRef->pNext;
    }

    if (bAsMake)
    {
      if (reader_write(" :"))
      {
        rc = RETURN_ERROR;
        break; /* while */
      }
      slen += 2;
      rc = output_list_flat(pRef, (int)slen, "   ", TRUE);
    }
    else
    {
      if (reader_write(" :\n"))
      {
        rc = RETURN_ERROR;
        break; /* while */
      }
      if (pRef)
        rc = output_list_flat(pRef, 0, "   <-", FALSE);
    }
    nodes_freelist(pList);
    slen = 0;

    /* If demanded, output the list of users as well */
    if (RETURN_OK == rc && !bAsMake)
    {
      pList = nodes_deplist(pNode, TRUE, bSelect);
      assert(pList);
      if (pList->pNext)
        rc = output_list_flat(pList->pNext, 0, "   ->", FALSE);
      nodes_freelist(pList);
    }

  } while(0);

  if (reader_writen("\n", 1))
    rc = RETURN_ERROR;

  return rc;
}

/*-------------------------------------------------------------------------*/
static int
output (int bAsMake)

/* Output the collected dependencies into the Makefile (bAsMake is TRUE)
 * resp. into the Depfile (bAsMake is FALSE).
 * Return 0 on success, RETURN_WARN on a mild error, RETURN_ERROR on a
 * severe error.
 */

{
  int       rc;
  char    * pBackup;  /* Name of the Makefilebackup */
  char    * pFile;    /* Name of file to create */
  Node    * pNode;

  pBackup = NULL;
  pFile = bAsMake ? sMake : sDepfile;

  assert(pFile);

  /* Rename old Makefile if necessary */
  if (bAsMake && bMakeExists)
  {
    pBackup = (char *)malloc(strlen(sMake)+5);
    strcpy(pBackup, sMake);
    strcat(pBackup, ".bak");
    remove(pBackup);
    if (rename(sMake, pBackup))
    {
      perror(aPgmName);
      printf("%s: Can't rename '%s' to '%s'.\n", aPgmName, sMake, pBackup);
      free(pBackup);
      return RETURN_ERROR;
    }
  }

  rc = RETURN_OK;
  do
  {
    char * pTagline;

    /* Open files */
    if (reader_openrw(pBackup, pFile))
    {
      perror(aPgmName);
      printf("%s: Can't write '%s'\n", aPgmName, pFile);
      rc = RETURN_ERROR;
      break;
    }

    if (bVerbose)
    {
      printf(" %s '%s'\r", (bMakeExists && bAsMake) ? "updating" : "creating", pFile);
      fflush(stdout);
    }

    /* Copy the Makefile up to the tagline */
    pTagline = IS_SELECT_MODE 
               ? "# --- DO NOT MODIFY THIS LINE -- SELECTED AUTO-DEPENDS FOLLOW ---\n"
               : "# --- DO NOT MODIFY THIS LINE -- AUTO-DEPENDS FOLLOW ---\n";
    if (bAsMake && reader_copymake(pTagline))
    {
      int i = errno;
      if (bVerbose)
        printf("%-78s\r", "");
      errno = i;
      perror(aPgmName);
      printf("%s: Error copying '%s' to '%s'.\n", aPgmName, pBackup, sMake);
      rc = RETURN_ERROR;
      break;
    }

    /* Walk and output the dependencies and users.
     * First all the files specified as root.
     */
    nodes_initwalk();
    while (rc != RETURN_ERROR && NULL != (pNode = nodes_inorder()) )
    {
      if ((pNode->flags & (NODE_SOURCE|NODE_AVOID|NODE_NOTFND)) == NODE_SOURCE)
        rc = output_node_flat(pNode, bAsMake, IS_SELECT_MODE);
    }  /* while (treewalk */

    /* Second (if wanted), all the files not specified as root. */
    if (RETURN_OK == rc && !bAsMake)
    {
      nodes_initwalk();
      while (rc != RETURN_ERROR && NULL != (pNode = nodes_inorder()) )
      {
        if ((pNode->flags & (NODE_SOURCE|NODE_AVOID)) != NODE_SOURCE
         && !(pNode->flags & NODE_NOTFND)
           )
          rc = output_node_flat(pNode, bAsMake, IS_SELECT_MODE);
      }  /* while (treewalk */
    }

    if (rc == RETURN_ERROR)
    {
      int i = errno;
      if (bVerbose)
        printf("%-78s\r", "");
      errno = i;
      perror(aPgmName);
      printf("%s: Error writing '%s'.\n", aPgmName, pFile);
      rc = RETURN_ERROR;
      break;
    }

    pTagline = IS_SELECT_MODE 
               ? "# --- DO NOT MODIFY THIS LINE -- SELECTED AUTO-DEPENDS PRECEDE ---\n"
               : "# --- DO NOT MODIFY THIS LINE -- AUTO-DEPENDS PRECEDE ---\n";
    if (bAsMake && reader_copymake2(pTagline))
    {
      int i = errno;
      if (bVerbose)
        printf("%-78s\r", "");
      errno = i;
      perror(aPgmName);
      printf("%s: Error copying '%s' to '%s'.\n", aPgmName, pBackup, sMake);
      rc = RETURN_ERROR;
      break;
    }
    else if (!bAsMake &&  reader_writeflush())
    {
      int i = errno;
      if (bVerbose)
        printf("%-78s\r", "");
      errno = i;
      perror(aPgmName);
      printf("%s: Error writing '%s'.\n", aPgmName, sDepfile);
      rc = RETURN_ERROR;
      break;
    }
    /* Finish up */
    if (bVerbose)
      printf("%-78s\r", "");

    if (reader_close())
    {
      perror(aPgmName);
      printf("%s: Error writing '%s'.\n", aPgmName, pFile);
      rc = RETURN_ERROR;
      break;
    }

    if (bAsMake && bMakeExists && chmod(sMake, aMakeStat.st_mode))
    {
      /* perror(aPgmName); */
      printf("%s: Warning: Can't update mode of '%s'.\n", aPgmName, sMake);
      rc = RETURN_WARN;
    }

    if (pBackup && remove(pBackup))
    {
      perror(aPgmName);
      printf("%s: Warning: Can't remove backup '%s'.\n", aPgmName, pBackup);
      rc = RETURN_WARN;
      break;
    }

  } while(0);

  /* Error cleanup */
  if (rc == RETURN_ERROR)
  {
    reader_close();
    remove(pFile);
    if (pBackup && rename(pBackup, sMake))
    {
      perror(aPgmName);
      printf("%s: Can't restore '%s' from backup '%s'\n", aPgmName, sMake, pBackup);
    }
  }

  if (bVerbose && rc != RETURN_ERROR)
    printf("%-78s\r", "");

  if (pBackup)
    free(pBackup);

  return rc;
}

/*-------------------------------------------------------------------------*/
int main (int argc, char *argv[])

{
  int i;

  reader_init();

  /* Get arguments, set up defaults */
  parseargs(argc, argv);
  if (!aFiles.size)
  {
    if (add_expfile(&aFiles, "*.c"))
      exit_nomem(" (main: add_expfile)");
  }
  if (!sObjExt)
    sObjExt = ".o";

  if (!aSrcExt.size)
  {
    if (array_addfile(&aSrcExt, ".c") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".cc") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".cp") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".cpp") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".cxx") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".C") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".CC") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".CP") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".CPP") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
    if (array_addfile(&aSrcExt, ".CXX") || array_addfile(&aObjExt, NULL))
      exit_nomem(" (main: add suffix)");
  }

  if (bVerbose)
  {
    printf("MkDepend %s -- Make Dependency Generator\n", VERSION);
    putchar('\n');
  }

  /* Look for the Makefile to modify */
  if (sMake)
  {
    bMakeExists = !stat(sMake, &aMakeStat);
  }
  else if (!sDepfile)
  {
    sMake = "Makefile";
    bMakeExists = !stat(sMake, &aMakeStat);
    if (!bMakeExists)
    {
      sMake = "makefile";
      bMakeExists = !stat(sMake, &aMakeStat);
    }
    if (!bMakeExists)
    {
      sMake = "Makefile.mk";
      bMakeExists = !stat(sMake, &aMakeStat);
    }
    if (!bMakeExists)
    {
      sMake = "makefile.mk";
      bMakeExists = !stat(sMake, &aMakeStat);
    }
    if (!bMakeExists)
    {
      sMake = "GNUMakefile";
      bMakeExists = !stat(sMake, &aMakeStat);
    }
    if (!bMakeExists)
      sMake = "Makefile";
  }

  /* Add the source files to the tree */
  if (!aFiles.size)
  {
    printf("%s: No files given.\n", aPgmName);
    set_rc(RETURN_WARN, TRUE);
  }
  else
  {
      struct stat aStat; /* buffer for stat() */

      for (i = 0; i < aFiles.size; i++)
      {
          if (stat(aFiles.strs[i], &aStat))
          {
              printf("%s: Warning: Can't find source '%s'.\n", aPgmName
                    , aFiles.strs[i]);
              set_rc(RETURN_WARN, FALSE);
          }
          else if (nodes_addsource(aFiles.strs[i], FALSE))
              exit_nomem(" (main: add source)");
      }

      /* If this leaves us with no source files at all, readfiles()
       * will complain.
       */
  }

  /* Mark the exceptional files */
  for (i = 0; i < aAvoid.size; i++)
  {
    if (nodes_addsource(aAvoid.strs[i], TRUE))
      exit_nomem(" (main: add avoided source)");
  }

  /* Read and analyse all those files */
  set_rc(readfiles(), 0);

  /* Handle the selection of files */
  if (IS_SELECT_MODE)
  {
    Node * pNode;

    /* First, mark all nodes with selected files */
    for (i = 0; i < aSelect.size; i++)
    {
      pNode = nodes_find(aSelect.strs[i]);
      if (pNode)
        pNode->flags |= NODE_SELECT;
    }

    /* Now walk the tree and propagate the select information */
    nodes_initwalk();
    while (NULL != (pNode = nodes_inorder()) )
    {
      if (!(pNode->flags & (NODE_IGNORE)) 
       && (pNode->flags & (NODE_SOURCE|NODE_AVOID)) == NODE_SOURCE)
      {
        NODES_UNMARK(pNode);
        nodes_mark_select(pNode);
      }
    }
  }

  if (returncode < RETURN_ERROR && sMake)
    set_rc(output(TRUE), 0);

  if (returncode < RETURN_ERROR && sMake && bVerbose)
    printf("%s '%s'.\n", bMakeExists ? "Updated" : "Created", sMake);

  if (returncode < RETURN_ERROR && sDepfile)
    set_rc(bFlat ? output(FALSE) : output_tree(), 0);

  if (returncode < RETURN_ERROR && sDepfile && bVerbose)
    printf("Created '%s'.\n", sDepfile);

  return (returncode == RETURN_WARN && bNoWarnExit) ? RETURN_OK : returncode;
}

/***************************************************************************/
