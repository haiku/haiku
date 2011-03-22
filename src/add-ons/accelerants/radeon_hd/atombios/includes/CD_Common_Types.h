/*
 * Copyright 2006-2007 Advanced Micro Devices, Inc.  
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*++

Module Name:

    CD_Common_Types.h
    
Abstract:

		Defines common data types to use across platforms/SW components

Revision History:

	NEG:17.09.2002	Initiated.
--*/
#ifndef _COMMON_TYPES_H_
	#define _COMMON_TYPES_H_

#if defined (__HAIKU__)
	// It's how we roll
	#include <stdint.h>
#endif

#ifndef	UEFI_BUILD
		typedef signed int			intn_t;
		typedef unsigned int		uintn_t;
#else
#ifndef EFIX64
		typedef signed int			intn_t;
		typedef unsigned int		uintn_t;
#endif
#endif


#ifndef VOID
typedef void		VOID;
#endif
#ifndef	UEFI_BUILD
	typedef intn_t		INTN;
	typedef uintn_t		UINTN;
#else
#ifndef EFIX64
	typedef intn_t		INTN;
	typedef uintn_t		UINTN;
#endif
#endif
#ifndef BOOLEAN
typedef uint8_t		BOOLEAN;
#endif
#ifndef INT8
typedef int8_t		INT8;
#endif
#ifndef UINT8
typedef uint8_t		UINT8;
#endif
#ifndef INT16
typedef int16_t		INT16;
#endif
#ifndef UINT16
typedef uint16_t	UINT16;
#endif
#ifndef INT32
typedef int32_t		INT32;
#endif
#ifndef UINT32
typedef uint32_t	UINT32;
#endif
//typedef int64_t   INT64;
//typedef uint64_t  UINT64;
typedef uint8_t		CHAR8;
typedef uint16_t	CHAR16;
#ifndef USHORT
typedef UINT16		USHORT;
#endif
#ifndef UCHAR
typedef UINT8		UCHAR;
#endif
#ifndef ULONG
typedef	UINT32		ULONG;
#endif

#ifndef _WIN64
#ifndef ULONG_PTR
typedef unsigned long ULONG_PTR;
#endif // ULONG_PTR
#endif // _WIN64

//#define	FAR	__far
#ifndef TRUE
  #define TRUE  ((BOOLEAN) 1 == 1)
#endif

#ifndef FALSE
  #define FALSE ((BOOLEAN) 0 == 1)
#endif

#ifndef NULL
  #define NULL  ((VOID *) 0)
#endif

//typedef	UINTN		CD_STATUS;


#endif // _COMMON_TYPES_H_

// EOF
