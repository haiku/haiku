/*
 * Copyright (c) 2003 Marcus Overhagen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef _BLOCK_CACHE_H
#define _BLOCK_CACHE_H


#include <BeBuild.h>
#include <Locker.h>


// The allocation type to be used in the constructor
enum {
	B_OBJECT_CACHE = 0,
	B_MALLOC_CACHE = 1
};

class BBlockCache {
	public:
		BBlockCache(uint32 blockCount, size_t blockSize,
			uint32 allocationType);
		virtual	~BBlockCache();

		void*	Get(size_t blockSize);
		void	Save(void *pointer, size_t blockSize);

	private:
		virtual	void _ReservedBlockCache1();
		virtual	void _ReservedBlockCache2();

		BBlockCache(const BBlockCache &);
		BBlockCache	&operator=(const BBlockCache &);

		struct _FreeBlock;

		_FreeBlock*	fFreeList;
		size_t		fBlockSize;
		int32		fFreeBlocks;
		int32		fBlockCount;
		BLocker		fLocker;
		void*		(*fAlloc)(size_t size);
		void		(*fFree)(void *pointer);
		uint32		_reserved[2];
};

#endif	// _BLOCK_CACHE_H
