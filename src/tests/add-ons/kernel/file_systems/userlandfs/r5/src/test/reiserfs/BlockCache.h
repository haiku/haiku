// BlockCache.h
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

#ifndef BLOCK_CACHE_H
#define BLOCK_CACHE_H

#include <SupportDefs.h>

#include "List.h"
#include "Locker.h"

class Block;
class Node;

class BlockCache {
public:
	BlockCache();
	~BlockCache();

	status_t Init(int device, uint64 blockCount, uint32 blockSize,
				  bool disableDeviceCache, size_t cacheCapacity);

	int GetDevice() const { return fDevice; }
	uint32 GetBlockSize() const { return fBlockSize; }
	off_t GetBlockCount() const { return fBlockCount; }

	status_t GetBlock(Block *block);
	status_t GetBlock(uint64 number, Block **block);
	void PutBlock(Block *block);

private:
	Block *_FindBlock(uint64 number);
	status_t _ReadBlock(uint64 number, Block **block);
	status_t _GetFreeBlock(Block **block);
	int32 _GetCacheSize();


friend class Block;
void *_GetBlock(off_t number) const;
void _ReleaseBlock(off_t number, void *data) const;

private:
	int						fDevice;
	uint32					fBlockSize;
	uint64					fBlockCount;
	BLocker					fLock;
	List<Block*>			fBlocks;
	int64					fBlockAge;
	bool					fNoDeviceCache;
	uint32					fCacheCapacity;
	// statistics
	int64					fReads;
	mutable int64			fBlockGets;
	mutable int64			fBlockReleases;
};

#endif	// BLOCK_CACHE_H
