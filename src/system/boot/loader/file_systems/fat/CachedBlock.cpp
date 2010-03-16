/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include "CachedBlock.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <util/kernel_cpp.h>


using namespace FATFS;


CachedBlock::CachedBlock(Volume &volume)
	:
	fVolume(volume),
	fBlockNumber(-1LL),
	fBlock(NULL)
{
}


CachedBlock::CachedBlock(Volume &volume, off_t block)
	:
	fVolume(volume),
	fBlockNumber(-1LL),
	fBlock(NULL)
{
	SetTo(block);
}


CachedBlock::~CachedBlock()
{
	free(fBlock);
}


uint8 *
CachedBlock::SetTo(off_t block)
{
	status_t error = SetTo(block, READ);
	return error == B_OK ? fBlock : NULL;
}


status_t
CachedBlock::SetTo(off_t blockNumber, uint32 flags)
{
	if (fBlock == NULL) {
		fBlock = (uint8*)malloc(BlockSize());
		if (fBlock == NULL)
			return B_NO_MEMORY;
	}

	if (blockNumber != fBlockNumber)
		flags |= FORCE;

	fBlockNumber = blockNumber;

	status_t error = B_OK;

	if ((flags & READ) != 0) {
		if ((flags & FORCE) != 0) {
			ssize_t bytesRead = read_pos(fVolume.Device(),
				fBlockNumber << BlockShift(), fBlock, BlockSize());
			if (bytesRead < 0)
				error = bytesRead;
			else if (bytesRead < (ssize_t)BlockSize())
				error = B_ERROR;
		}
	} else if ((flags & CLEAR) != 0)
		memset(fBlock, 0, BlockSize());

	if (error != B_OK)
		fBlockNumber = -1;

	return error;
}


status_t
CachedBlock::Flush()
{
	if (fBlockNumber < 0)
		return B_BAD_VALUE;

	ssize_t written = write_pos(fVolume.Device(), fBlockNumber << BlockShift(),
		fBlock, BlockSize());
	if (written < 0)
		return errno;
	if (written != (ssize_t)BlockSize())
		return B_ERROR;

	return B_OK;
}
