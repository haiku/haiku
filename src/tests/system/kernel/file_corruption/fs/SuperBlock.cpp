/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SuperBlock.h"

#include <string.h>

#include "Volume.h"


bool
SuperBlock::Check(uint64 totalBlocks) const
{
	if (strncmp(signature1, CHECK_SUM_FS_SIGNATURE_1,
			kCheckSumFSSignatureLength) != 0
		|| strncmp(signature2, CHECK_SUM_FS_SIGNATURE_2,
			kCheckSumFSSignatureLength) != 0
		|| Version() != kCheckSumFSVersion
		|| TotalBlocks() < kCheckSumFSMinSize / B_PAGE_SIZE
		|| TotalBlocks() > totalBlocks
		|| strnlen(name, kCheckSumFSNameLength) >= kCheckSumFSNameLength) {
		return false;
	}
	// TODO: Check free blocks and location of root directory and block bitmap.

	return true;
}


void
SuperBlock::Initialize(Volume* volume)
{
	memcpy(signature1, CHECK_SUM_FS_SIGNATURE_1, kCheckSumFSSignatureLength);
	memcpy(signature2, CHECK_SUM_FS_SIGNATURE_2, kCheckSumFSSignatureLength);

	version = kCheckSumFSVersion;
	totalBlocks = volume->TotalBlocks();
//	freeBlocks = volume->BlockAllocator()->FreeBlocks();
//	blockBitmap = volume->BlockAllocator()->BlockBitmap();
//	rootDir = volume->RootDirBlock();
	strlcpy(name, volume->Name(), kCheckSumFSNameLength);
}
