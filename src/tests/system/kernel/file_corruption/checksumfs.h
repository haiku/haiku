/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHECK_SUM_FS_H
#define CHECK_SUM_FS_H


#include <OS.h>


#define CHECK_SUM_FS_PRETTY_NAME	"CheckSum File System"


static const uint64 kCheckSumFSSuperBlockOffset = 16 * B_PAGE_SIZE;
static const uint64 kCheckSumFSMinSize
	= kCheckSumFSSuperBlockOffset + 16 * B_PAGE_SIZE;


static const uint32 kCheckSumFSNameLength		= 256;

static const uint32 kCheckSumFSSignatureLength	= 16;
#define CHECK_SUM_FS_SIGNATURE_1	"_1!cHEcKsUmfS!1_"
#define CHECK_SUM_FS_SIGNATURE_2	"-2@ChECkSumFs@2-"

static const uint32 kCheckSumFSVersion = 1;

struct checksumfs_super_block {
	char	signature1[kCheckSumFSSignatureLength];
	uint32	version;
	uint32	pad1;
	uint64	totalBlocks;
	uint64	freeBlocks;
	uint64	rootDir;
	uint64	blockBitmap;
	char	name[kCheckSumFSNameLength];
	char	signature2[kCheckSumFSSignatureLength];
} _PACKED;


#endif	// CHECK_SUM_FS_H
