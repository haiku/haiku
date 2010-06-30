/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <new>

#include <fs_cache.h>

#include "Block.h"
#include "BlockAllocator.h"
#include "checksumfs.h"
#include "SuperBlock.h"


Volume::Volume(uint32 flags)
	:
	fFD(-1),
	fFlags(flags),
	fBlockCache(NULL),
	fTotalBlocks(0),
	fName(NULL),
	fBlockAllocator(NULL)
{
}


Volume::~Volume()
{
	delete fBlockAllocator;

	if (fBlockCache != NULL)
		block_cache_delete(fBlockCache, false);

	if (fFD >= 0)
		close(fFD);

	free(fName);
}


status_t
Volume::Init(const char* device)
{
	// open the device
	fFD = open(device, IsReadOnly() ? O_RDONLY : O_RDWR);
	if (fFD < 0)
		return errno;

	// get the size
	struct stat st;
	if (fstat(fFD, &st) < 0)
		return errno;

	return _Init(st.st_size / B_PAGE_SIZE);
}


status_t
Volume::Init(int fd, uint64 totalBlocks)
{
	fFD = dup(fd);
	if (fFD < 0)
		return errno;

	return _Init(totalBlocks);
}


status_t
Volume::Mount()
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
Volume::Initialize(const char* name)
{
	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	status_t error = fBlockAllocator->Initialize();
	if (error != B_OK)
		return error;

	Block block;
	if (!block.GetZero(this, kCheckSumFSSuperBlockOffset / B_PAGE_SIZE))
		return B_ERROR;

	SuperBlock* superBlock = (SuperBlock*)block.Data();
	superBlock->Initialize(this);

	block.Put();

	return block_cache_sync(fBlockCache);
}


status_t
Volume::_Init(uint64 totalBlocks)
{
	fTotalBlocks = totalBlocks;
	if (fTotalBlocks * B_PAGE_SIZE < kCheckSumFSMinSize)
		return B_BAD_VALUE;

	// create a block cache
	fBlockCache = block_cache_create(fFD, fTotalBlocks, B_PAGE_SIZE,
		IsReadOnly());
	if (fBlockCache == NULL)
		return B_NO_MEMORY;

	// create the block allocator
	fBlockAllocator = new(std::nothrow) BlockAllocator(this);
	if (fBlockAllocator == NULL)
		return B_NO_MEMORY;

	return B_OK;
}
