//
// File:			BlockCache.cpp
//
// Description:		Handles a cache of memory blocks of the same size
//
// Copyright (c) 2002 Massimiliano Origgi
//
// License:			See MIT license
//

#include <BlockCache.h>

#include <Autolock.h>

#include <stdlib.h>


// Private functions

void *object_alloc(size_t Size)
{
	return (void *)(new char[Size]);
}

void object_free(void *Data)
{
	delete[] Data;
}

void *malloc_alloc(size_t Size)
{
	return malloc(Size);
}

void malloc_free(void *Data)
{
	free(Data);
}

// Private structure for cache
struct _Block
{
	_Block(void);
	~_Block(void);
	void *Data;
	bool InUse;
};

_Block::_Block(void) :
	Data(NULL),
	InUse(false)
{
}

_Block::~_Block(void)
{
}


BBlockCache::BBlockCache(size_t CacheSize, size_t BlockSize, uint32 Type) :
	fCacheSize(CacheSize),
	fBlockSize(BlockSize),
	fMark(0)
{
	// Setup function pointers based on Type
	if(Type==B_OBJECT_CACHE) {
		fAlloc=&object_alloc;
		fFree=&object_free;
	} else {
		fAlloc=&malloc_alloc;
		fFree=&malloc_free;
	}
	
	// Allocate cache
	fCache=(void *)new _Block[CacheSize];
	for(size_t i=0; i<CacheSize; i++) {
		((_Block *)fCache)[i].Data=fAlloc(BlockSize);
	}
}


BBlockCache::~BBlockCache()
{
	delete[] fCache;
}

void *
BBlockCache::Get(size_t BlockSize)
{
	BAutolock lock(fLock);
	
	if(BlockSize!=fBlockSize) {
		return fAlloc(BlockSize);
	}
	
	_Block *block;
	for(size_t i=fMark; i<fCacheSize; i++) {
		block=&((_Block *)fCache)[i];
		if(block->InUse==false) {
			block->InUse=true;
			++fMark;
			if(fMark==fCacheSize) fMark=0;
			return block->Data;
		}
	}
	
	if(fMark==0) return fAlloc(BlockSize);
	
	for(size_t i=0; i<fMark; i++) {
		block=&((_Block *)fCache)[i];
		if(block->InUse==false) {
			block->InUse=true;
			++fMark;
			if(fMark==fCacheSize) fMark=0;
			return block->Data;
		}
	}
	
	return fAlloc(BlockSize);
}

void 
BBlockCache::Save(void *Data, size_t BlockSize)
{
	BAutolock lock(fLock);
	
	if(BlockSize!=fBlockSize) {
		fFree(Data);
		return;
	}

	_Block *block;
	for(size_t i=0; i<fCacheSize; i++) {
		block=&((_Block *)fCache)[i];
		if(block->Data==Data) {
			block->InUse=false;
			fMark=i;
			return;
		}
	}
	fFree(Data);
}
