/*
	This file tests basic functionality of BBlockCache.
*/


#include "BlockCacheExerciseTest.h"

#include <stdlib.h>

#include <BlockCache.h>

#include "cppunit/TestCaller.h"


/*
 *  Method: BlockCacheExerciseTest::BlockCacheExerciseTest()
 *   Descr: This method is the only constructor for the BlockCacheExerciseTest
 *          class.
 */
BlockCacheExerciseTest::BlockCacheExerciseTest(std::string name)
	: 
	TestCase(name),
	theCache(NULL),
	numBlocksInCache(0),
	sizeOfBlocksInCache(0),
	sizeOfNonCacheBlocks(0),
	isMallocTest(false)
{
}


/*
 *  Method: BlockCacheExerciseTest::~BlockCacheExerciseTest()
 *   Descr: This method is the destructor for the BlockCacheExerciseTest class.
 */
BlockCacheExerciseTest::~BlockCacheExerciseTest()
{
}


/*
 *  Method:  BlockCacheExerciseTest::TestBlockCache()
 *   Descr:  This method performs the tests on a particular BBlockCache.
 *           The goal of the method is to perform the following basic
 *           sequence:
 *             1. Get (numBlocksInCache + 10) blocks of "cache size" from the
 *                BBlockCache.  By doing this, we can guarantee that the cache
 *                has been exhausted and some elements are allocated to
 *                satisfy the caller.  Then, they are all returned to the
 *                cache in most recent block to oldest block order.  This
 *                confirms that regardless whether the block was initially
 *                part of the cache or created because the cache was empty,
 *                once it is returned to the cache, it is just placed on the
 *                cache.  Imagine a cache of 4 blocks.  Initally, the cache
 *                has the following blocks:
 *                   A, B, C, D
 *                If five blocks are gotten from the cache, the caller will get
 *                   A, B, C, D, E
 *                However, E wasn't initially part of the cache.  It was allocated
 *                dynamically to satisfy the caller's request because the cache
 *                was empty.  Now, if they are returned in the order E, D, C, B, A
 *                the cache will have the following blocks available in it:
 *                   B, C, D, E
 *                When A is returned, the cache will find there is no more room
 *                for more free blocks and it will be freed.  This is the
 *                behaviour which is confirmed initially.
 *
 *             2. After this is done, the cache is just "exercised".  The following
 *                is done "numBlocksInCache" times:
 *                  - 4 "cache sized" blocks are gotten from the cache
 *                  - 4 "non-cache sized" blocks are gotten from the cache
 *                  - 1 "cache sized" block is returned back to the cache
 *                  - 1 "non-cache sized" block is returned back to the cache
 *                  - 1 "cache sized" block is freed and not returned to the cache
 *                  - 1 "non-cache sized" block is freed and not returned to the
 *                    cache (but even if given to the BBlockCache, it would just
 *                    be freed anyhow)
 *                 What this means is that everytime through the loop, 2 "cache sized"
 *                 and 2 "non-cache sized" blocks are kept in memory.  At the end, 
 *                 2 * numBlocksInCache items of size "cache size" and "non cache size"
 *                 will exist.
 *
 *                 Then, numBlocksInCache / 4 items are returned to the cache and
 *                 numBlocksInCache / 4 are freed.  This ensures at the end of the
 *                 test that there are some available blocks in the cache.
 *                
 *           The sum total of these actions test the BBlockCache.
 */
void
BlockCacheExerciseTest::TestBlockCache(void)
{

	// First get all items from the cache plus ten more
	for (int i = 0; i < numBlocksInCache + 10; i++) {
		GetBlock(sizeOfBlocksInCache);
	}
	
	// Put them all back in reverse order to confirm 1 from above
	while (!usedList.IsEmpty()) {
		SaveBlock(usedList.LastItem(), sizeOfBlocksInCache);
	}
	
	// Get a bunch of blocks and send some back to the cache
	// to confirm 2 from above.
	for (int i = 0; i < numBlocksInCache; i++) {
		GetBlock(sizeOfBlocksInCache);
		GetBlock(sizeOfBlocksInCache);
		GetBlock(sizeOfNonCacheBlocks);
		GetBlock(sizeOfNonCacheBlocks);
	
		// We send one back from the middle of the lists so
		// the most recent block is not the one returned.
		SaveBlock(usedList.ItemAt(usedList.CountItems() / 2),
		          sizeOfBlocksInCache);
		SaveBlock(nonCacheList.ItemAt(nonCacheList.CountItems() / 2),
		          sizeOfNonCacheBlocks);
		
		GetBlock(sizeOfBlocksInCache);
		GetBlock(sizeOfBlocksInCache);
		GetBlock(sizeOfNonCacheBlocks);
		GetBlock(sizeOfNonCacheBlocks);
		
		// We free one from the middle of the lists so the
		// most recent block is not the one freed.
		FreeBlock(usedList.ItemAt(usedList.CountItems() / 2),
		          sizeOfBlocksInCache);
		FreeBlock(nonCacheList.ItemAt(nonCacheList.CountItems() / 2),
		          sizeOfNonCacheBlocks);
		}
	
	// Now, send some blocks back to the cache and free some blocks
	// so the cache is not empty at the end of the test.
	for (int i = 0; i < numBlocksInCache / 4; i++) {
		// Return the blocks which are 2/3s of the way through the
		// lists.
		SaveBlock(usedList.ItemAt(usedList.CountItems() * 2 / 3),
		          sizeOfBlocksInCache);
		SaveBlock(nonCacheList.ItemAt(nonCacheList.CountItems() * 2 / 3),
		          sizeOfNonCacheBlocks);
		
		// Free the blocks which are 1/3 of the way through the lists.
		FreeBlock(usedList.ItemAt(usedList.CountItems() / 3),
		          sizeOfBlocksInCache);
		FreeBlock(nonCacheList.ItemAt(nonCacheList.CountItems() / 3),
		          sizeOfNonCacheBlocks);
	}
}


/*
 *  Method:  BlockCacheExerciseTest::BuildLists()
 *   Descr:  This method gets all of the blocks from the cache in order to
 *           track them for future tests.  The assumption here is that the
 *           blocks are not deallocated and reallocated in the implementation
 *           of BBlockCache.  Given the purpose is to provide a cheap way to
 *           access allocated memory without resorting to malloc()'s or new's,
 *           this should be a fair assumption.
 */
void
BlockCacheExerciseTest::BuildLists()
{
	freeList.MakeEmpty();
	usedList.MakeEmpty();
	nonCacheList.MakeEmpty();
	
	for(int i = 0; i < numBlocksInCache; i++) {
		freeList.AddItem(theCache->Get(sizeOfBlocksInCache));
	}
	for(int i = 0; i < numBlocksInCache; i++) {
		theCache->Save(freeList.ItemAt(i), sizeOfBlocksInCache);
	}
}


/*
 *  Method:  BlockCacheExerciseTest::GetBlock()
 *   Descr:  This method returns a pointer from the BBlockCache, checking
 *           the value before passing it to the caller.
 */
void *
BlockCacheExerciseTest::GetBlock(size_t blockSize)
{
	void *thePtr = theCache->Get(blockSize);
	
	// This new pointer should not be one which we already
	// have from the BBlockCache which we haven't given back
	// yet.
	CPPUNIT_ASSERT(!usedList.HasItem(thePtr));
	CPPUNIT_ASSERT(!nonCacheList.HasItem(thePtr));
	
	if (blockSize == sizeOfBlocksInCache) {
		// If this block was one which could have come from the
		// cache and there are free items on the cache, it
		// should be one of those free blocks.
		if (freeList.CountItems() > 0) {
			CPPUNIT_ASSERT(freeList.RemoveItem(thePtr));
		}
		CPPUNIT_ASSERT(usedList.AddItem(thePtr));
	} else {
		// A "non-cache sized" block should never come from the
		// free list.
		CPPUNIT_ASSERT(!freeList.HasItem(thePtr));
		CPPUNIT_ASSERT(nonCacheList.AddItem(thePtr));
	}
	return(thePtr);
}


/*
 *  Method:  BlockCacheExerciseTest::SavedCacheBlock()
 *   Descr:  This method passes the pointer back to the BBlockCache
 *           and checks the sanity of the lists.
 */
void
BlockCacheExerciseTest::SaveBlock(void *thePtr, size_t blockSize)
{
	// The memory block being returned to the cache should
	// not already be free.
	CPPUNIT_ASSERT(!freeList.HasItem(thePtr));
	
	if (blockSize == sizeOfBlocksInCache) {
		// If there is room on the free list, when this block
		// is returned to the cache, it will be put on the
		// free list.  Therefore we will also track it as
		// a free block on the cache.
		if (freeList.CountItems() < numBlocksInCache) {
			CPPUNIT_ASSERT(freeList.AddItem(thePtr));
		}
		
		// This block should not be on the non-cache list but it
		// should be on the used list.
		CPPUNIT_ASSERT(!nonCacheList.HasItem(thePtr));
		CPPUNIT_ASSERT(usedList.RemoveItem(thePtr));
	} else {
		// This block should not be on the used list but it should
		// be on the non-cache list.
		CPPUNIT_ASSERT(!usedList.HasItem(thePtr));
		CPPUNIT_ASSERT(nonCacheList.RemoveItem(thePtr));
	}
	theCache->Save(thePtr, blockSize);
}


/*
 *  Method:  BlockCacheExerciseTest::FreeBlock()
 *   Descr:  This method frees the block directly using delete[] or free(),
 *           checking the sanity of the lists as it does the operation.
 */
void
BlockCacheExerciseTest::FreeBlock(void *thePtr, size_t blockSize)
{
	// The block being freed should not already have been
	// returned to the cache.
	CPPUNIT_ASSERT(!freeList.HasItem(thePtr));
	
	if (blockSize == sizeOfBlocksInCache) {
		// This block should not be on the non-cache list but it
		// should be on the used list.
		CPPUNIT_ASSERT(!nonCacheList.HasItem(thePtr));
		CPPUNIT_ASSERT(usedList.RemoveItem(thePtr));
	} else {
		// This block should not be on the used list but it should
		// be on the non-cache list.
		CPPUNIT_ASSERT(!usedList.HasItem(thePtr));
		CPPUNIT_ASSERT(nonCacheList.RemoveItem(thePtr));
	}
	if (isMallocTest) {
		free(thePtr);
	} else {
		delete[] (uint8*)thePtr;
	}
}


/*
 *  Method:  BlockCacheExerciseTest::PerformTest()
 *   Descr:  This method performs the tests on a series of BBlockCache
 *           instances.  It performs the tests with a series of
 *           block sizes and numbers of blocks.  Also, it does the
 *           test using a B_OBJECT_CACHE and B_MALLOC_CACHE type
 *           cache.  For each individual BBlockCache to be tested, it:
 *             1. Queries the cache to find out the blocks which are
 *                on it.
 *             2. Performs the test.
 *             3. Frees all blocks left after the test to prevent
 *                memory leaks.
 */
void
BlockCacheExerciseTest::PerformTest(void)
{
	for (numBlocksInCache = 8; numBlocksInCache < 513; numBlocksInCache *= 2) {
		for (sizeOfBlocksInCache = 13; sizeOfBlocksInCache < 9478; sizeOfBlocksInCache *= 3) {
			
			// To test getting blocks which are not from the cache,
			// we will get blocks of 6 bytes less than the size of
			// the blocks on the cache.
			sizeOfNonCacheBlocks = sizeOfBlocksInCache - 6;
			
			isMallocTest = false;
			theCache = new BBlockCache(numBlocksInCache, sizeOfBlocksInCache, B_OBJECT_CACHE);
			CPPUNIT_ASSERT(theCache != NULL);
			
			// Query the cache and determine the blocks in it.
			BuildLists();
			// Perform the test on this instance.
			TestBlockCache();
			delete theCache;
			// Clean up remaining memory.
			while (!usedList.IsEmpty()) {
				FreeBlock(usedList.LastItem(), sizeOfBlocksInCache);
			}
			while (!nonCacheList.IsEmpty()) {
				FreeBlock(nonCacheList.LastItem(), sizeOfNonCacheBlocks);
			}
	
			isMallocTest = true;
			theCache = new BBlockCache(numBlocksInCache, sizeOfBlocksInCache, B_MALLOC_CACHE);
			CPPUNIT_ASSERT(theCache != NULL);
	
			// Query the cache and determine the blocks in it.
			BuildLists();
			// Perform the test on this instance.
			TestBlockCache();
			delete theCache;
			// Clean up remaining memory.
			while (!usedList.IsEmpty()) {
				FreeBlock(usedList.LastItem(), sizeOfBlocksInCache);
			}
			while (!nonCacheList.IsEmpty()) {
				FreeBlock(nonCacheList.LastItem(), sizeOfNonCacheBlocks);
			}
		}
	}
}


/*
 *  Method:  BlockCacheExerciseTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "BlockCacheExerciseTest" test.
 */
CppUnit::Test *BlockCacheExerciseTest::suite()
{	
	typedef CppUnit::TestCaller<BlockCacheExerciseTest>
		BlockCacheExerciseTestCaller;
		
	return(new BlockCacheExerciseTestCaller("BBlockCache::Exercise Test", &BlockCacheExerciseTest::PerformTest));
}



