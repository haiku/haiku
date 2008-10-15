/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */


#include "CachedBlock.h"
#include "Stream.h"
#include "Directory.h"
#include "File.h"

#include <util/kernel_cpp.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>


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


inline void
CachedBlock::Unset()
{
	fBlockNumber = -1;
}


inline uint8 *
CachedBlock::SetTo(off_t block)
{
	if (block == fBlockNumber)
		return fBlock;
	if (fBlock == NULL) {
		fBlock = (uint8 *)malloc(BlockSize());
		if (fBlock == NULL)
			return NULL;
	}

	fBlockNumber = block;
	if (read_pos(fVolume.Device(), block << BlockShift(), fBlock, BlockSize()) < (ssize_t)BlockSize())
		return NULL;

	return fBlock;
}

