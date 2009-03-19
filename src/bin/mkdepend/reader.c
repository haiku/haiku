/* $Id: reader.c 1.9 Wed, 01 Mar 2000 22:36:57 -0700 lars $ */

/*---------------------------------------------------------------------------
** Reading of C-Sources and filter for #include statements.
** Also here are routines for the modification of Makefiles.
**
** Copyright (c) 1995-1998 by Lars Düning. All Rights Reserved.
** This files is free software. For terms of use see the file LICENSE.
**---------------------------------------------------------------------------
** Credits:
**   The buffering scheme was inspired by the input module of lcc (by
**   Chris Fraser & David Hanson).
**---------------------------------------------------------------------------
*/

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "reader.h"

/*-------------------------------------------------------------------------*/

struct filestate
 {
  int len;      /* Number of bytes buffered */
  char buf[1];  /* allocated to <len> bytes */
 };

#define BUFSIZE 10240              /* Size of the file buffer */
static char   aBuffer[BUFSIZE+1];  /* File buffer */
static int    fdFile;              /* Filehandle */
static int    fdFile2;             /* Filehandle Writer */
static char * pLimit;              /* Pointer to closing sentinel in buffer */
static char * pch;                 /* Pointer to actual char in buffer */
static char * pIName;              /* Start of include filename */
static struct filestate * pState;  /* Saved state of the Makefile */

#define END_OF_BUF()  (pch >= pLimit)
#define END_OF_FILE() (pLimit == aBuffer)

/*-------------------------------------------------------------------------*/
void
reader_init (void)

/* Initialize the reader.
 */

{
  fdFile = -1;
  fdFile2 = -1;
  aBuffer[BUFSIZE] = '\n';
  pLimit = aBuffer+BUFSIZE;
  pch = pLimit;
  pIName = NULL;
  pState = NULL;
}

/*-------------------------------------------------------------------------*/
static int
fillbuffer (void)

/* Read in the next chunk from file such that incrementing pch will
 * access the newly read data.
 *
 * Result:
 *   0 on success, non-0 else (errno holds error code then).
 *   pch is set to the start of the new data.
 */

{
  int iRead;

  if (END_OF_FILE())
    return 1;
  if (pIName)
  {
    if (pch > pLimit)
      pch = pLimit;
    memcpy(aBuffer, pIName, (unsigned)(pLimit-pIName));
    pch -= (pLimit-pIName);
  }
  else
    pch = aBuffer;
  iRead = read(fdFile, pch, (unsigned)(aBuffer+BUFSIZE-pch));
  if (iRead >= 0)
  {
    pLimit = pch+iRead;
    *pLimit = '\n';
  }
  return iRead < 0;
}

/*-------------------------------------------------------------------------*/
int
reader_open (const char *pName)

/* Open a file for reading.
 *
 *   pName: Name of the file to read.
 *
 * Result:
 *   0 on success, non-0 else (errno contains error code then).
 *
 * The reader is initialized to read and filter the file name *pName.
 */

{
  assert(pName);
  assert(fdFile < 0);
  reader_init();
  fdFile = open(pName, O_RDONLY);
  return fdFile >= 0;
}

/*-------------------------------------------------------------------------*/
int
reader_openrw (const char *pNameR, const char *pNameW)

/* Open two makefiles for modification.
 *
 *   pNameR: Name of the file to read, may be NULL.
 *   pNameW: Name of the file to read.
 *
 * Result:
 *   0 on success, non-0 else (errno contains error code then).
 *
 * The reader is initialized to read the old makefile *pNameR and writing
 * of new makefile *pNameW.
 * If no pNameR is given, the reader is immediately initialized to
 * write *pNameW.
 */

{
  assert(pNameW);
  assert(fdFile < 0);
  assert(fdFile2 < 0);
  reader_init();
  fdFile2 = open(pNameW, O_WRONLY|O_CREAT|O_TRUNC, 0664);
  if (fdFile2 < 0)
    return 1;
  if (pNameR)
  {
    fdFile = open(pNameR, O_RDONLY);
    if (fdFile < 0)
    {
      close(fdFile2);
      fdFile2 = -1;
    }
  }
  else
    pch = aBuffer;
  return fdFile2 < 0;
}

/*-------------------------------------------------------------------------*/
int
reader_close (void)

/* Close the file currently read.
 * Result:
 *   0 on success, non-0 else.
 *   errno set to error code in case.
 */

{
  int i;

  i = 0;
  if (fdFile >= 0)
  {
    i = close(fdFile);
  }
  fdFile = -1;
  if (fdFile2 >= 0)
  {
    i = close(fdFile2) || i;
  }
  fdFile2 = -1;
  if (pState)
  {
    free(pState);
  }
  pState = NULL;
  return i;
}

/*-------------------------------------------------------------------------*/
int
reader_eof (void)

/* Test if the file read is at its end.
 *
 * Result:
 *   Non-0 on end of file.
 */

{
  return END_OF_FILE();
}

/*-------------------------------------------------------------------------*/
const char *
reader_get (int * pType)

/* Extract the next include statement from the file being read.
 *
 * Result:
 *   Pointer to the filename extracted from the statement, or NULL on error
 *     or end of file.
 *     The memory is property of reader_get(), the pointer valid just until
 *     the next call to reader_get().
 *   <*pType> is set to 0 for an ""-include, and non-zero for an <>-include.
 *   <errn>o set to an error code in case.
 */

{
  enum { StartOfLine, InLine, FoundHash, SkipComment, SkipLComment, FoundInclude
      }  eState;
  /* For debugging purposes:
  char   *stab[]
    = {
        "Start "
      , "InLine"
      , "Found#"
      , "SkipC "
      , "SkipLC"
      , "FoundI"
      };
  */
  char   ch;
  int    iFound; /* Number of chars found of "include" */
  int    bDone;  /* 0: still searching, 1: found include */
  int    bType;  /* 0: ""-include, 1: <>-include */

#define GETCHAR() \
  ch = *pch++; \
  if (ch == '\n' && pch >= pLimit) \
  { \
    if (fillbuffer()) { \
      pIName = NULL; \
      return NULL; \
    } \
    ch = *pch++; \
  }

  if (pIName)
    eState = InLine;
  else
    eState = StartOfLine;
  pIName = NULL;

  iFound = 0;
  bType = 0;
  bDone = 0;
  while (!bDone)
  {
    /* Get the next interesting non-linefeed character */
    do {
      GETCHAR()
      if (ch == '\n' || ch == '\r' || ch == '\v' || ch == '\f')
      {
        eState = StartOfLine;
        pIName = NULL;
      }
      else
        break;
    } while (1);

    switch(eState)
    {
    case StartOfLine:
      switch (ch)
      {
      case '#':
        eState = FoundHash;
        iFound = 0;
        break;
      case '/':
        GETCHAR()
        if (ch == '/')
          eState = SkipLComment;
        else if (ch == '*')
          eState = SkipComment;
        else
          eState = InLine;
        break;
      case ' ': case '\t':
        /* still: eState = StartOfLine; */
        break;
      default:
        eState = InLine;
        break;
      }
      break;

    case InLine:
      switch (ch)
      {
      case '/':
        GETCHAR()
        if (ch == '/')
          eState = SkipLComment;
        else if (ch == '*')
          eState = SkipComment;
        else
          eState = InLine;
        break;
      case '\\':
        GETCHAR()
        /* still: eState = InLine; */
        break;
      default:
        /* still: eState = InLine; */
        break;
      }
      break;

    case FoundHash:
      switch(ch)
      {
      case ' ': case '\t':
        if (iFound)
        {
          eState = InLine;
          iFound = 0;
        }
        /* else still: eState = FoundHash */
        break;
      case 'i': case 'n': case 'c': case 'l':
      case 'u': case 'd': case 'e':
        if ("include"[iFound] != ch)
        {
          eState = InLine;
          iFound = 0;
        }
        else if (ch == 'e')
        {
          eState = FoundInclude;
          pIName = NULL;
        }
        else
          iFound++;
        break;
      default:
        eState = InLine;
      }
      break;

    case FoundInclude:
      switch(ch)
      {
      case ' ': case '\t':
        /* still: eState = FoundInclude */
        break;
      case '<':
        if (!pIName)
        {
          pIName = pch;
          bType = 1;
        }
        else
          eState = InLine;
        break;
      case '>':
        if (!pIName)
        {
          eState = InLine;
        }
        else if (bType)
        {
          *(pch-1) = '\0';
          if (pch-1 == pIName)
            pIName = NULL;
          else
            bDone = 1;
          eState = InLine;
        }
        break;
      case '"':
        if (!pIName)
        {
          pIName = pch;
          bType = 0;
        }
        else if (!bType)
        {
          *(pch-1) = '\0';
          if (pch-1 == pIName)
            pIName = NULL;
          else
            bDone = 1;
          eState = InLine;
        }
        break;

      default:
        if (!pIName)
          eState = InLine;
        break;
      }
      break;

    case SkipComment:
    case SkipLComment:
      break;
    } /* switch (eState) */

    /* Skip comments immediately */
    if (eState == SkipComment)
    {
      while(1) {
        do {
          GETCHAR()
        } while (ch != '*');
        GETCHAR()
        if (ch == '/')
        break;
      }
      eState = StartOfLine;
    }
    else if (eState == SkipLComment)
    {
      do {
        GETCHAR()
      } while (ch != '\n' && ch != '\r' && ch != '\v' && ch != '\f');
      eState = StartOfLine;
    }
  } /* while(!bDone) */

  *pType = bType;
  return pIName;

#undef GETCHAR
}

/*-------------------------------------------------------------------------*/
int
reader_writeflush (void)

/* Write out the buffer to the written makefile.
 * Return 0 on success, non-0 else (errno holds the error code then).
 */

{
  if (pch > aBuffer
   && write(fdFile2, aBuffer, (unsigned)(pch-aBuffer)) != pch-aBuffer)
    return 1;
  pch = aBuffer;
  return 0;
}

/*-------------------------------------------------------------------------*/
int
reader_writen (const char *pText, size_t len)

/* Append the first <len> characters of <pText> to the written file.
 * Return 0 on success, non-0 else (errno holds the error code then)
 */

{
  assert(pText);
  assert(fdFile2 >= 0);
  if (pLimit-pch < (signed)len && reader_writeflush())
    return 1;
  memcpy(pch, pText, len);
  pch += len;
  return 0;
}

/*-------------------------------------------------------------------------*/
int
reader_write (const char *pText)

/* Append <pText> to the written file.
 * Return 0 on success, non-0 else (errno holds the error code then)
 */

{
  return reader_writen(pText, strlen(pText));
}

/*-------------------------------------------------------------------------*/
int
reader_copymake (const char * pTagline)

/* Copy the file from fdFile to fdFile2 up to including the tagline.
 * If the tagline is not found, it is appended.
 * The tagline MUST end in a newline character!
 * Return 0 on success, non-0 on error (errno holds the error code then).
 */

{
  size_t len;
  int count; /* Next charactor to compare in pTagline, or -1 if to skip
              * up to the next Newline */
  char ch;


  assert(pTagline);
  assert(fdFile2);
  len = strlen(pTagline);
  assert(len < 120);

  /* Simplest case: no old makefile to copy */
  if (fdFile < 0)
  {
    strcpy(aBuffer, pTagline);
    pch = aBuffer+len;
    return 0;
  }
  if (fillbuffer())
    return 1;
  count = 0;

  while (!END_OF_FILE() && count < (signed)len)
  {
    if (END_OF_BUF())
    {
      if (write(fdFile2, aBuffer, (unsigned)(pch-aBuffer)) != pch-aBuffer)
        return 1;
      if (fillbuffer())
        return 1;
      if (END_OF_FILE())
        break;
    }
    ch = *pch;
    if (count < 0)
    {
      if (ch == '\n')
        count = 0;
    }
    else
    {
      if (ch == pTagline[count])
        count++;
      else if (ch == '\n')
        count = 0;
      else
        count = -1;
    }
    pch++;
  }

  /* Remember the state of the Makefile read */
  if (!END_OF_FILE())
  {
    pState = (struct filestate *) malloc(sizeof(*pState)+(pLimit-pch));
    if (pState)
    {
      pState->len = pLimit-pch;
      memcpy(pState->buf, pch, (size_t)pState->len);
    }
  }

  /* Switch to write mode */
  pLimit = aBuffer+BUFSIZE;

  /* Append a newline if missing */
  if (count < 0 && reader_writen("\n", 1))
    return 1;

  /* Put the Tagline into the buffer if necessary */
  if (count < (signed)len && reader_writen(pTagline, (size_t)len))
    return 1;

  return 0;
}

/*-------------------------------------------------------------------------*/
int
reader_copymake2 (const char * pTagline)

/* Copy the remainder from fdFile to fdFile2 starting with the tagline.
 * The tagline MUST end in a newline character!
 * Return 0 on success, non-0 on error (errno holds the error code then).
 */

{
  size_t len;
  int count; /* Next charactor to compare in pTagline, or -1 if to skip
              * up to the next Newline */
  char ch;


  assert(pTagline);
  assert(fdFile);
  assert(fdFile2);
  len = strlen(pTagline);
  assert(len < 120);

  /* Append the tagline to the file written */
  if (reader_write(pTagline) || reader_writeflush())
    return 1;

  /* Easiest case: nothing to left copy */
  if (!pState)
    return 0;

  /* Restore the aBuffer to the point where the reading stopped */
  pLimit = aBuffer+BUFSIZE;
  pch = pLimit-pState->len;
  memcpy(pch, pState->buf, (size_t)pState->len);
  free(pState);
  pState = NULL;
  pIName = NULL;

  /* Fill up the buffer */
  if (END_OF_BUF() && fillbuffer())
    return 1;

  /* Skip the Makefile read until the tagline has been found */
  count = 0;
  while (!END_OF_FILE() && count < (signed)len)
  {
    if (END_OF_BUF())
    {
      if (fillbuffer())
        return 1;
      if (END_OF_FILE())
        break;
    }
    ch = *pch;
    if (count < 0)
    {
      if (ch == '\n')
        count = 0;
    }
    else
    {
      if (ch == pTagline[count])
        count++;
      else if (ch == '\n')
        count = 0;
      else
        count = -1;
    }
    pch++;
  }

  /* Copy the remainder of the Makefile */
  do
  {
    if (!END_OF_BUF()
     && write(fdFile2, pch, (unsigned)(pLimit-pch)) != pLimit-pch)
      return 1;
    if (!END_OF_FILE() && fillbuffer())
      return 1;
  }
  while (!END_OF_FILE());

  return 0;
}

/***************************************************************************/
