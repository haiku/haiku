/*
 * Copyright 2007, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VMDKHEADER_H
#define _VMDKHEADER_H

typedef unsigned char uint8;
typedef unsigned int  uint32;
typedef unsigned long long uint64;


typedef uint64 SectorType;
typedef uint8 Bool;

struct SparseExtentHeader
{
	uint32            magicNumber;
	uint32            version;
	uint32            flags;
	SectorType        capacity;
	SectorType        grainSize;
	SectorType        descriptorOffset;
	SectorType        descriptorSize;
	uint32            numGTEsPerGT;
	SectorType        rgdOffset;
	SectorType        gdOffset;
	SectorType        overHead;
	Bool              uncleanShutdown;
	char              singleEndLineChar;
	char              nonEndLineChar;
	char              doubleEndLineChar1;
	char              doubleEndLineChar2;
	uint8             pad[435];
} __attribute__((__packed__)) ;

#define SPARSE_MAGICNUMBER 0x564d444b /* 'V' 'M' 'D' 'K' */

#endif
