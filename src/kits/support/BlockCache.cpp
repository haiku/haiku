//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		BlockCache.cpp
//	Author(s):		Massimiliano Origgi
//
//	Description:	Handles a cache of memory blocks of the same size  
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <Autolock.h>
#include <BlockCache.h>


// Private functions
static void *
object_alloc(size_t Size)
{
	return (void *)(new char[Size]);
}


static void
object_free(void *Data)
{
	delete[] (int8*)Data;
}


static void*
malloc_alloc(size_t Size)
{
	return malloc(Size);
}


static void
malloc_free(void *Data)
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


BBlockCache::BBlockCache(size_t CacheSize, size_t BlockSize, uint32 Type)
	:	fCacheSize(CacheSize),
		fBlockSize(BlockSize),
		fMark(0)
{
	// Setup function pointers based on Type
	if(Type == B_OBJECT_CACHE) {
		fAlloc = &object_alloc;
		fFree = &object_free;
	} else {
		fAlloc = &malloc_alloc;
		fFree = &malloc_free;
	}
	
	// Allocate cache
	fCache = (void *)new _Block[CacheSize];
	
	for(size_t i = 0; i < CacheSize; i++)
		((_Block *)fCache)[i].Data = fAlloc(BlockSize);
}


BBlockCache::~BBlockCache()
{
	delete[] (_Block*)fCache;
}


void *
BBlockCache::Get(size_t BlockSize)
{
	BAutolock lock(fLock);
	
	if(BlockSize != fBlockSize)
		return fAlloc(BlockSize);
	
	_Block *block;
	for(size_t i = fMark; i < fCacheSize; i++) {
		block = &((_Block *)fCache)[i];
		if(block->InUse == false) {
			block->InUse = true;
			++fMark;
			if(fMark == fCacheSize)
				fMark = 0;
			return block->Data;
		}
	}
	
	if(fMark == 0)
		return fAlloc(BlockSize);
	
	for(size_t i = 0; i < fMark; i++) {
		block = &((_Block *)fCache)[i];
		if(block->InUse == false) {
			block->InUse = true;
			++fMark;
			if(fMark == fCacheSize)
				fMark = 0;
			return block->Data;
		}
	}
	
	return fAlloc(BlockSize);
}


void 
BBlockCache::Save(void *Data, size_t BlockSize)
{
	BAutolock lock(fLock);
	
	if(BlockSize != fBlockSize) {
		fFree(Data);
		return;
	}

	_Block *block;
	for(size_t i = 0; i < fCacheSize; i++) {
		block = &((_Block *)fCache)[i];
		if(block->Data == Data) {
			block->InUse = false;
			fMark = i;
			return;
		}
	}
	fFree(Data);
}
