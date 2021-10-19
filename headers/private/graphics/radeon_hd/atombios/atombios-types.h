/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _ATOMBIOS_TYPES_H
#define _ATOMBIOS_TYPES_H


#include <SupportDefs.h>


#if defined(__POWERPC__)
#define ATOM_BIG_ENDIAN 1
#else
#define ATOM_BIG_ENDIAN 0
#endif


// Align types to Haiku's types
typedef uint16 USHORT;
typedef uint32 ULONG;
typedef uint8 UCHAR;


#endif /* _ATOMBIOS_TYPES_H */
