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

#include <new>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "BlockCache.h"
#include "Block.h"
#include "cache.h"
#include "Debug.h"
#include "reiserfs.h"

using std::nothrow;

/*!
	\class BlockCache
	\brief Implements a cache for disk blocks.

	The BlockCache can work in two different modes. The first one uses the
	system device block cache and the object is object doesn't do much more
	than two maintain a list of Blocks currently being in use. The second
	mode does a bit more. It implements a cache with a simple strategy
	dropping least-recently-used items.

	The second mode is useful for userland tests and, due to a bug in the
	BeOS when dealing with 4096 bytes sized blocks, also for the real thing.

	The central methods are GetBlock() and PutBlock(). The former one
	requests a certain block and the latter tells the cache, that the caller
	is done with the block. When a block is unused it is either kept in
	memory until the next time it is needed or until its memory is needed
	for another block.
*/

// constructor
BlockCache::BlockCache()
	: fDevice(-1),
	  fBlockSize(0),
	  fBlockCount(0),
	  fLock(),
	  fBlocks(),
	  fBlockAge(0),
	  fNoDeviceCache(true),
	  fCacheCapacity(0),
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
			INFORM(("WARNING: block not put: %p (ref count: %ld)\n", block,
					block->_GetRefCount()));
		}
		PRINT(("statistics: block %p: age: %Ld\n", block, block->_GetAge()));
		delete block;
	}
	PRINT(("statistics: %Ld block reads\n", fReads));
	PRINT(("statistics: %Ld block gets\n", fBlockGets));
	PRINT(("statistics: %Ld block releases\n", fBlockReleases));
	if (!fNoDeviceCache)
		remove_cached_device_blocks(fDevice, NO_WRITES);
	fLock.Unlock();
}

// Init
status_t
BlockCache::Init(int device, uint64 blockCount, uint32 blockSize,
				 bool disableDeviceCache, uint32 cacheCapacity)
{
	status_t error = (device >= 0 && blockSize > 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fDevice = device;
		fBlockSize = blockSize;
		fBlockCount = blockCount;
		fCacheCapacity = cacheCapacity / fBlockSize;
		fNoDeviceCache = true;
		// init device cache
		if (error == B_OK && !disableDeviceCache) {
			if (init_cache_for_device(fDevice, blockCount) == B_OK) {
				fNoDeviceCache = false;
				fCacheCapacity = 0;	// release blocks, when unused
			} else {
				INFORM(("WARNING: init_cache_for_device() failed, use "
						"built-in caching\n"));
			}
		}
	}
	RETURN_ERROR(error);
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
	status_t error = (result && number < fBlockCount ? B_OK : B_BAD_VALUE);
	fLock.Lock();
	// find the block in the cache
	Block *block = NULL;
	if (error == B_OK) {
		block = _FindBlock(number);
		if (!block) {
			// not found, read it from disk
			error = _ReadBlock(number, &block);
			if (error == B_OK)
				fBlocks.AddItem(block);
		}
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
		if (block->_Put() && block->_GetAge() < fBlockAge) {
			block->_SetAge(fBlockAge);
			fBlockAge++;
			// free the block, if the cache is over full
			if (_GetCacheSize() > (int32)fCacheCapacity) {
				fBlocks.RemoveItem(block);
				delete block;
			}
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
	Block *block = NULL;
	status_t error = _GetFreeBlock(&block);
	if (error == B_OK)
		error = block->_SetTo(this, number);
	// set the result / cleanup on error
	if (error == B_OK)
		*result = block;
	else if (block)
		delete block;
	return error;
}

// _GetFreeBlock
status_t
BlockCache::_GetFreeBlock(Block **result)
{
	status_t error = B_OK;
	Block *unusedBlock = NULL;
	// Search for an unused block, but only, if there's no device cache, since
	// there are no unused blocks otherwise.
	if (fNoDeviceCache && _GetCacheSize() >= (int32)fCacheCapacity) {
		// cache is full, try to reuse a block
		int64 age = fBlockAge;
		for (int32 i = 0; Block *block = fBlocks.ItemAt(i); i++) {
			if (block->_GetRefCount() == 0 && block->_GetAge() < age) {
				unusedBlock = block;
				age = block->_GetAge();
			}
		}
		if (unusedBlock)
			fBlocks.RemoveItem(unusedBlock);
		else {
			INFORM(("WARNING: Cache is full and all blocks are in use: "
					"size: %ld\n", _GetCacheSize()));
		}
	}
	if (!unusedBlock) {
		// allocate a block
		unusedBlock = new(nothrow) Block;
		if (!unusedBlock)
			error = B_NO_MEMORY;
	}
	if (error == B_OK)
		*result = unusedBlock;
	return error;
}

// _GetCacheSize
int32
BlockCache::_GetCacheSize()
{
	return fBlocks.CountItems();
}

// _GetBlock
void *
BlockCache::_GetBlock(off_t number) const
{
	fBlockGets++;	// statistics
	void *data = NULL;
	if (fNoDeviceCache) {
		data = malloc(fBlockSize);
		if (data) {
			if (read_pos(fDevice, number * fBlockSize, data, fBlockSize)
				!= (ssize_t)fBlockSize) {
				free(data);
				data = NULL;
				PRINT(("reading block %Ld failed: %s\n",
					   number, strerror(errno)));
			}
		}
	} else
		data = get_block(fDevice, number, fBlockSize);
	return data;
}

// _ReleaseBlock
void
BlockCache::_ReleaseBlock(off_t number, void *data) const
{
	fBlockReleases++;	// statistics
	if (fNoDeviceCache)
		free(data);
	else {
		status_t error = release_block(fDevice, number);
		if (error != B_OK) {
			FATAL(("release_block(%d, %Ld) failed: %s\n", fDevice, number,
				   strerror(error)));
		}
	}
}

