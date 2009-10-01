/*
 * Copyright 2007, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VMDK_H
#define _VMDK_H


#include <stdint.h>


typedef uint64_t SectorType;


struct SparseExtentHeader {
	uint32_t		magicNumber;
	uint32_t		version;
	uint32_t		flags;
	SectorType		capacity;
	SectorType		grainSize;
	SectorType		descriptorOffset;
	SectorType		descriptorSize;
	uint32_t		numGTEsPerGT;
	SectorType		rgdOffset;
	SectorType		gdOffset;
	SectorType		overHead;
	uint8_t			uncleanShutdown;
	char			singleEndLineChar;
	char			nonEndLineChar;
	char			doubleEndLineChar1;
	char			doubleEndLineChar2;
	uint8_t			pad[435];
} __attribute__((__packed__)) ;

#define VMDK_SPARSE_MAGICNUMBER	0x564d444b /* 'V' 'M' 'D' 'K' */
#define VMDK_SPARSE_VERSION		1

#endif	// _VMDK_H
