/* $Id: util.c 1.2 Wed, 01 Mar 2000 21:23:27 -0700 lars $ */

/*---------------------------------------------------------------------------
 * Useful functions.
 *
 * Copyright (c) 1999 by Lars Düning. All Rights Reserved.
 * This file is free software. For terms of use see the file LICENSE.
 *---------------------------------------------------------------------------
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
 
#include "util.h"

/*-------------------------------------------------------------------------*/
char *
util_getpath (const char * pDirtyName, char ** ppBasename)

/* Take the <pDirtyName> and eliminate as many '.' and '..' from it
 * as possible. Return a freshly allocated copy of the cleaned up path.
 * If ppBasename is NULL, the returned name will only contain the
 * directory parts, the filename will be dropped. If ppBasename is not
 * NULL, the returned name will contain the filename, and *ppBasename
 * will be set to point to its first character.
 *
 * Return NULL when out of memory.
 */

{
    char * pPath;          /* The newly created pathname */
    char * pStart;         /* The first non-'/'-character of pPath */
    char * pSrc, * pDest;  /* Working pointers */
    size_t oFilename;      /* Offset of filename in pDirtyName */

    assert(pDirtyName);

    pPath = strdup(pDirtyName);
    if (!pPath)
        return NULL;

    pStart = ('/' == *pPath) ? pPath+1 : pPath;

    /* First scan: find the first character of the filename
     * and end the string there, leaving just the directory path.
     * But remember the offset of the filename.
     */
    pDest = pStart;  /* First character after last '/' found */
    pSrc = pStart;   /* Rover */
    while ('\0' != *pSrc)
    {
        if ('/' == *pSrc++)
            pDest = pSrc;
    }
    *pDest = '\0';
    oFilename = (unsigned)(pDest - pPath);

    /* pPath now ends in a '/', or *pStart is a '\0' */

    /* Second scan: visit all directory parts and eliminate '.'
     * and '..' where possible.
     */
    pDest = pStart;  /* First character after last valid '/' */
    pSrc = pStart;

    /* Skip initial '.' entries */
    while ('.' == *pSrc)
    {
        if ('/' == pSrc[1])
        {
            pSrc += 2;
        }
        else
            break;
    }

    while ('\0' != *pSrc)
    {
        char ch;

        /* Copy forward until the next '/' */
        do
        {
             ch = *pSrc++;
            *pDest++ = ch;
        } while ('/' != ch);

        /* If a '.' follows, it might be a '.' or '..' entry */
        while ('.' == *pSrc)
        {
            pSrc++;
            
            if ('/' == *pSrc) 
            {
                /* Simply skip the '.' entry */
                pSrc++;
            }
            else if ('.' == pSrc[0] && '/' == pSrc[1] && pDest != pStart)
            {
                /* For a '..' entry, move pDest back one directory unless
                 * it is at the start of the string
                 */
                char * pLastDest;
                
                for ( pLastDest = pDest-1
                    ; pLastDest != pStart && '/' != pLastDest[-1]
                    ; pLastDest--) /* SKIP */;

                /* Special case: if the part just unrolled is '..' itself,
                 * don't do the unrolling and instead break the loop.
                 */
                if (pLastDest + 3 == pDest && pLastDest[0] == '.' && pLastDest[1] == '.')
                {
                    *pDest++ = '.';
                    break;
                }
                else
                {
                    pSrc += 2;
                    pDest = pLastDest;
                }
            }
            else
            {
                /* Its not a '.' or handleable '..' entry */
                *pDest++ = '.';
                break;
            }
        }
    }
    *pDest = '\0';

    /* If the caller wants to have the filename too, append it to
     * the clean directory.
     */
    if (ppBasename)
    {
        strcpy(pDest, pDirtyName+oFilename);
        *ppBasename = pDest;
    }
    
    return pPath;
}

/***************************************************************************/
