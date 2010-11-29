/*
	This file tests BBlockCache from multiple threads to ensure there are
	no concurrency problems.
*/


#include "BlockCacheConcurrencyTest.h"

#include <stdlib.h>

#include <BlockCache.h>
#include <List.h>

#include "ThreadedTestCaller.h"


/*
 *  Method: BlockCacheConcurrencyTest::BlockCacheConcurrencyTest()
 *   Descr: This method is the only constructor for the BlockCacheConcurrencyTest
 *          class.
 */
BlockCacheConcurrencyTest::BlockCacheConcurrencyTest(std::string name)
	:
	BThreadedTestCase(name),
	theObjCache(NULL),
	theMallocCache(NULL),
	numBlocksInCache(128),
	sizeOfBlocksInCache(23),
	sizeOfNonCacheBlocks(29)
{
}


/*
 *  Method: BlockCacheConcurrencyTest::~BlockCacheConcurrencyTest()
 *   Descr: This method is the destructor for the BlockCacheConcurrencyTest class.
 */
BlockCacheConcurrencyTest::~BlockCacheConcurrencyTest()
{
}


/*
 *  Method:  BlockCacheConcurrencyTest::setUp()
 *   Descr:  This method creates a couple of BBlockCache instances to perform
 *           tests on.  One uses new/delete and the other uses malloc/free
 *           on its blocks.
 */
void
BlockCacheConcurrencyTest::setUp()
{
	theObjCache = new BBlockCache(numBlocksInCache, sizeOfBlocksInCache,
		B_OBJECT_CACHE);
	theMallocCache = new BBlockCache(numBlocksInCache, sizeOfBlocksInCache,
		B_MALLOC_CACHE);
}


/*
 *  Method:  BlockCacheConcurrencyTest::tearDown()
 *   Descr:  This method cleans up the BBlockCache instances which were tested.
 */
void
BlockCacheConcurrencyTest::tearDown()
{
	delete theObjCache;
	delete theMallocCache;
}


/*
 *  Method:  BlockCacheConcurrencyTest::GetBlock()
 *   Descr:  This method returns a pointer from the BBlockCache, checking
 *           the value before passing it to the caller.
 */
void *
BlockCacheConcurrencyTest::GetBlock(BBlockCache *theCache, size_t blockSize,
	thread_id theThread, BList *cacheList, BList *nonCacheList)
{
	void *thePtr = theCache->Get(blockSize);

	// The new block should not already be used by this thread.
	CPPUNIT_ASSERT(!cacheList->HasItem(thePtr));
	CPPUNIT_ASSERT(!nonCacheList->HasItem(thePtr));

	// Add the block to the list of blocks used by this thread.
	if (blockSize == sizeOfBlocksInCache) {
		CPPUNIT_ASSERT(cacheList->AddItem(thePtr));
	} else {
		CPPUNIT_ASSERT(nonCacheList->AddItem(thePtr));
	}

	// Store the thread id at the start of the block for future
	// reference.
	*((thread_id *)thePtr) = theThread;
	return(thePtr);
}


/*
 *  Method:  BlockCacheConcurrencyTest::SavedCacheBlock()
 *   Descr:  This method passes the pointer back to the BBlockCache
 *           and checks the sanity of the lists.
 */
void
BlockCacheConcurrencyTest::SaveBlock(BBlockCache *theCache, void *thePtr,
	size_t blockSize, thread_id theThread, BList *cacheList,
	BList *nonCacheList)
{
	// The block being returned to the cache should still have
	// the thread id of this thread in it, or some other thread has
	// perhaps manipulated this block which would indicate a
	// concurrency problem.
	CPPUNIT_ASSERT(*((thread_id *)thePtr) == theThread);

	// Remove the item from the appropriate list and confirm it isn't
	// on the other list for some reason.
	if (blockSize == sizeOfBlocksInCache) {
		CPPUNIT_ASSERT(cacheList->RemoveItem(thePtr));
		CPPUNIT_ASSERT(!nonCacheList->HasItem(thePtr));
	} else {
		CPPUNIT_ASSERT(!cacheList->HasItem(thePtr));
		CPPUNIT_ASSERT(nonCacheList->RemoveItem(thePtr));
	}
	theCache->Save(thePtr, blockSize);
}


/*
 *  Method:  BlockCacheConcurrencyTest::FreeBlock()
 *   Descr:  This method frees the block directly using delete[] or free(),
 *           checking the sanity of the lists as it does the operation.
 */
void
BlockCacheConcurrencyTest::FreeBlock(void *thePtr, size_t blockSize,
	bool isMallocTest, thread_id theThread, BList *cacheList,
	BList *nonCacheList)
{
	// The block being returned to the cache should still have
	// the thread id of this thread in it, or some other thread has
	// perhaps manipulated this block which would indicate a
	// concurrency problem.
	CPPUNIT_ASSERT(*((thread_id *)thePtr) == theThread);

	// Remove the item from the appropriate list and confirm it isn't
	// on the other list for some reason.
	if (blockSize == sizeOfBlocksInCache) {
		CPPUNIT_ASSERT(cacheList->RemoveItem(thePtr));
		CPPUNIT_ASSERT(!nonCacheList->HasItem(thePtr));
	} else {
		CPPUNIT_ASSERT(!cacheList->HasItem(thePtr));
		CPPUNIT_ASSERT(nonCacheList->RemoveItem(thePtr));
	}
	if (isMallocTest) {
		free(thePtr);
	} else {
		delete[] (uint8*)thePtr;
	}
}


/*
 *  Method:  BlockCacheConcurrencyTest::TestBlockCache()
 *   Descr:  This method performs the tests on BBlockCache.  It is
 *           called by 6 threads concurrently.  Three of them are
 *           operating on the B_OBJECT_CACHE instance of BBlockCache
 *           and the other three are operating on the B_MALLOC_CACHE
 *           instance.
 *
 *           The goal of this method is to perform a series of get,
 *           save and free operations on block from the cache using
 *           "cache size" and "non-cache size" blocks.  Also, at the
 *           end of this method, all blocks unfreed by this method are
 *           freed to avoid a memory leak.
 */
void
BlockCacheConcurrencyTest::TestBlockCache(BBlockCache *theCache,
	bool isMallocTest)
{
	BList cacheList;
	BList nonCacheList;
	thread_id theThread = find_thread(NULL);

	// Do everything eight times to ensure the test runs long
	// enough to check for concurrency problems.
	for (int j = 0; j < 8; j++) {
		// Perform a series of gets, saves and frees
		for (int i = 0; i < numBlocksInCache / 2; i++) {
			GetBlock(theCache, sizeOfBlocksInCache, theThread, &cacheList, &nonCacheList);
			GetBlock(theCache, sizeOfBlocksInCache, theThread, &cacheList, &nonCacheList);
			GetBlock(theCache, sizeOfNonCacheBlocks, theThread, &cacheList, &nonCacheList);
			GetBlock(theCache, sizeOfNonCacheBlocks, theThread, &cacheList, &nonCacheList);

			SaveBlock(theCache, cacheList.ItemAt(cacheList.CountItems() / 2),
			          sizeOfBlocksInCache, theThread, &cacheList, &nonCacheList);
			SaveBlock(theCache, nonCacheList.ItemAt(nonCacheList.CountItems() / 2),
			          sizeOfNonCacheBlocks, theThread, &cacheList, &nonCacheList);

			GetBlock(theCache, sizeOfBlocksInCache, theThread, &cacheList, &nonCacheList);
			GetBlock(theCache, sizeOfBlocksInCache, theThread, &cacheList, &nonCacheList);
			GetBlock(theCache, sizeOfNonCacheBlocks, theThread, &cacheList, &nonCacheList);
			GetBlock(theCache, sizeOfNonCacheBlocks, theThread, &cacheList, &nonCacheList);

			FreeBlock(cacheList.ItemAt(cacheList.CountItems() / 2),
			          sizeOfBlocksInCache, isMallocTest, theThread, &cacheList, &nonCacheList);
			FreeBlock(nonCacheList.ItemAt(nonCacheList.CountItems() / 2),
			          sizeOfNonCacheBlocks, isMallocTest, theThread, &cacheList, &nonCacheList);
			}
		bool performFree = false;
		// Free or save (every other block) for all "cache sized" blocks.
		while (!cacheList.IsEmpty()) {
			if (performFree) {
				FreeBlock(cacheList.LastItem(), sizeOfBlocksInCache, isMallocTest, theThread, &cacheList,
				          &nonCacheList);
			} else {
				SaveBlock(theCache, cacheList.LastItem(), sizeOfBlocksInCache, theThread, &cacheList,
				          &nonCacheList);
			}
			performFree = !performFree;
		}
		// Free or save (every other block) for all "non-cache sized" blocks.
		while (!nonCacheList.IsEmpty()) {
			if (performFree) {
				FreeBlock(nonCacheList.LastItem(), sizeOfNonCacheBlocks, isMallocTest, theThread, &cacheList,
				          &nonCacheList);
			} else {
				SaveBlock(theCache, nonCacheList.LastItem(), sizeOfNonCacheBlocks, theThread, &cacheList,
				          &nonCacheList);
			}
			performFree = !performFree;
		}
	}
}


/*
 *  Method:  BlockCacheConcurrencyTest::TestThreadMalloc()
 *   Descr:  This method passes the BBlockCache instance to TestBlockCache()
 *           where the instance will be tested.
 */
void
BlockCacheConcurrencyTest::TestThreadMalloc()
{
	TestBlockCache(theMallocCache, true);
}


/*
 *  Method:  BlockCacheConcurrencyTest::TestThreadObj()
 *   Descr:  This method passes the BBlockCache instance to TestBlockCache()
 *           where the instance will be tested.
 */
void
BlockCacheConcurrencyTest::TestThreadObj()
{
	TestBlockCache(theObjCache, false);
}


/*
 *  Method:  BlockCacheConcurrencyTest::suite()
 *   Descr:  This static member function returns a test caller for performing
 *           the "BlockCacheConcurrencyTest" test.  The test caller
 *           is created as a ThreadedTestCaller with six independent threads.
 */
CppUnit::Test *BlockCacheConcurrencyTest::suite()
{
	typedef BThreadedTestCaller <BlockCacheConcurrencyTest >
		BlockCacheConcurrencyTestCaller;

	BlockCacheConcurrencyTest *theTest = new BlockCacheConcurrencyTest("");
	BlockCacheConcurrencyTestCaller *threadedTest = new BlockCacheConcurrencyTestCaller("BBlockCache::Concurrency Test", theTest);
	threadedTest->addThread("A", &BlockCacheConcurrencyTest::TestThreadObj);
	threadedTest->addThread("B", &BlockCacheConcurrencyTest::TestThreadObj);
	threadedTest->addThread("C", &BlockCacheConcurrencyTest::TestThreadObj);
	threadedTest->addThread("D", &BlockCacheConcurrencyTest::TestThreadMalloc);
	threadedTest->addThread("E", &BlockCacheConcurrencyTest::TestThreadMalloc);
	threadedTest->addThread("F", &BlockCacheConcurrencyTest::TestThreadMalloc);
	return(threadedTest);
}
