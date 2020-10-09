// BlockCache.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include "BlockCache.h"

#include <new>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fs_cache.h>

#include "Block.h"
#include "Debug.h"
#include "reiserfs.h"

using std::nothrow;

/*!
	\class BlockCache
	\brief Implements a cache for disk blocks.

	The central methods are GetBlock() and PutBlock(). The former one
	requests a certain block and the latter tells the cache, that the caller
	is done with the block. When a block is unused it is either kept in
	memory until the next time it is needed or until its memory is needed
	for another block.
*/

// constructor
BlockCache::BlockCache()
	:
	fDevice(-1),
	fBlockSize(0),
	fBlockCount(0),
	fLock(),
	fBlocks(),
	fReads(0),
	fBlockGets(0),
	fBlockReleases(0)
{
}

// destructor
BlockCache::~BlockCache()
{
	fLock.Lock();
	for (int32 i = 0; Block *block = fBlocks.ItemAt(i); i++) {
		if (block->_GetRefCount() > 0) {
			INFORM(("WARNING: block not put: %p (ref count: %" B_PRId32 ")\n",
				block, block->_GetRefCount()));
		}
		delete block;
	}
	PRINT(("statistics: %" B_PRId64 " block reads\n", fReads));
	PRINT(("statistics: %" B_PRId64 " block gets\n", fBlockGets));
	PRINT(("statistics: %" B_PRId64 " block releases\n", fBlockReleases));
	if (fCacheHandle)
		block_cache_delete(fCacheHandle, false);
	fLock.Unlock();
}

// Init
status_t
BlockCache::Init(int device, uint64 blockCount, uint32 blockSize)
{
	if (device < 0 || blockSize <= 0)
		return B_BAD_VALUE;

	fDevice = device;
	fBlockSize = blockSize;
	fBlockCount = blockCount;

	// init block cache
	fCacheHandle = block_cache_create(fDevice, blockCount, blockSize, true);
	if (!fCacheHandle)
		return B_ERROR;

	return B_OK;
}

// GetBlock
status_t
BlockCache::GetBlock(Block *block)
{
	status_t error = (block ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fLock.Lock();
		if (fBlocks.HasItem(block))
			block->_Get();
		else
			error = B_BAD_VALUE;
		fLock.Unlock();
	}
	return error;
}

// GetBlock
status_t
BlockCache::GetBlock(uint64 number, Block **result)
{

	if (!result || number >= fBlockCount)
		return B_BAD_VALUE;

	fLock.Lock();

	// find the block in the cache
	status_t error = B_OK;
	Block *block = _FindBlock(number);
	if (!block) {
		// not found, read it from disk
		error = _ReadBlock(number, &block);
		if (error == B_OK)
			fBlocks.AddItem(block);
	}

	// increase the block's reference counter
	if (error == B_OK) {
		block->_Get();
		*result = block;
	}

	fLock.Unlock();
	return error;
}

// PutBlock
void
BlockCache::PutBlock(Block *block)
{
	fLock.Lock();
	if (block && fBlocks.HasItem(block)) {
		if (block->_Put()) {
			fBlocks.RemoveItem(block);
			delete block;
		}
	}
	fLock.Unlock();
}

// _FindBlock
Block *
BlockCache::_FindBlock(uint64 number)
{
	for (int32 i = 0; Block *block = fBlocks.ItemAt(i); i++) {
		if (block->GetNumber() == number)
			return block;
	}
	return NULL;
}

// _ReadBlock
status_t
BlockCache::_ReadBlock(uint64 number, Block **result)
{
	fReads++;	// statistics

	// get a free block and read the block data
	Block *block = new(nothrow) Block;
	if (!block)
		return B_NO_MEMORY;

	status_t error = block->_SetTo(this, number);

	// set the result / cleanup on error
	if (error == B_OK)
		*result = block;
	else if (block)
		delete block;
	return error;
}

// _GetBlock
void *
BlockCache::_GetBlock(off_t number) const
{
	fBlockGets++;	// statistics
	return const_cast<void*>(block_cache_get(fCacheHandle, number));
}

// _ReleaseBlock
void
BlockCache::_ReleaseBlock(off_t number, void *data) const
{
	fBlockReleases++;	// statistics
	block_cache_put(fCacheHandle, number);
}

