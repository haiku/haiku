/*
*******************************************************************************
*
*   Copyright (C) 1999-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  utf.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep09
*   created by: Markus W. Scherer
*	adapted for use under BeOS by Axel Dörfler, axeld@pinc-software.de.
*/

#ifndef __UTF_H__
#define __UTF_H__

#include "UnicodeProperties.h"
#include <stddef.h>


#define UTF_SIZE 8
#define U_SIZEOF_UCHAR (UTF_SIZE>>3)


typedef uint32 UChar32;

#ifndef UTF_SAFE
#	define UTF_SAFE
#endif

/* internal definitions ----------------------------------------------------- */

#define UTF8_ERROR_VALUE_1 0x15
#define UTF8_ERROR_VALUE_2 0x9f
#define UTF_ERROR_VALUE 0xffff

/** Is this code unit or code point a surrogate (U+d800..U+dfff)? */
#define UTF_IS_SURROGATE(unichar) (((unichar)&0xfffff800)==0xd800)

/**	Is a given 32-bit code point/Unicode scalar value
 *	actually a valid Unicode (abstract) character?
 */
#define UTF_IS_UNICODE_CHAR(c) \
    ((uint32_t)(c)<=0x10ffff && \
     !UTF_IS_SURROGATE(c) && ((c)&0xfffe)!=0xfffe)

/**	Is a given 32-bit code an error value
 *	as returned by one of the macros for any UTF?
 */
#define UTF_IS_ERROR(c) \
    (((c)&0xfffe)==0xfffe || (c)==UTF8_ERROR_VALUE_1 || (c)==UTF8_ERROR_VALUE_2)

/** This is a combined macro: Is c a valid Unicode value _and_ not an error code? */
#define UTF_IS_VALID(c) \
    ((uint32_t)(c)<=0x10ffff && \
     !UTF_IS_SURROGATE(c) && \
     ((c)&0xfffe)!=0xfffe && \
     (c)!=UTF8_ERROR_VALUE_1 && (c)!=UTF8_ERROR_VALUE_2)

#include "utf8.h"

/*
 * ANSI C header:
 * limits.h defines CHAR_MAX
 */
#include <limits.h>

#define UTF_APPEND_CHAR_SAFE(s, i, length, c)	UTF8_APPEND_CHAR_SAFE(s, i, length, c)
#define UTF_APPEND_CHAR_UNSAFE(s, i, c)			UTF8_APPEND_CHAR_UNSAFE(s, i, c)


/* Define UChar to be compatible with char if possible. */
#if CHAR_MAX>=255
	typedef char UChar;
#else
	typedef uint8 UChar;
#endif


#define UTF_GET_CHAR(s, start, i, length, c) UTF_GET_CHAR_SAFE(s, start, i, length, c, FALSE)

#define UTF_NEXT_CHAR(s, i, length, c)       UTF_NEXT_CHAR_SAFE(s, i, length, c, FALSE)
#define UTF_APPEND_CHAR(s, i, length, c)     UTF_APPEND_CHAR_SAFE(s, i, length, c)
#define UTF_FWD_1(s, i, length)              UTF_FWD_1_SAFE(s, i, length)
#define UTF_FWD_N(s, i, length, n)           UTF_FWD_N_SAFE(s, i, length, n)
#define UTF_SET_CHAR_START(s, start, i)      UTF_SET_CHAR_START_SAFE(s, start, i)

#define UTF_PREV_CHAR(s, start, i, c)        UTF_PREV_CHAR_SAFE(s, start, i, c, FALSE)
#define UTF_BACK_1(s, start, i)              UTF_BACK_1_SAFE(s, start, i)
#define UTF_BACK_N(s, start, i, n)           UTF_BACK_N_SAFE(s, start, i, n)
#define UTF_SET_CHAR_LIMIT(s, start, i, length) UTF_SET_CHAR_LIMIT_SAFE(s, start, i, length)

#endif	/* __UTF_H__ */
