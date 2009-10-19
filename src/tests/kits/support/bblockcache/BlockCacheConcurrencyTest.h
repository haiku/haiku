/*
	$Id: BlockCacheConcurrencyTest.h 4522 2003-09-07 11:53:03Z bonefish $
	
	This file defines a class for testing BBlockCache
	
	*/


#ifndef BlockCacheConcurrencyTest_H
#define BlockCacheConcurrencyTest_H


#include "ThreadedTestCase.h"
#include <string>
#include <OS.h>


class BBlockCache;
class BList;


class BlockCacheConcurrencyTest : public BThreadedTestCase {
	
private:
	BBlockCache *theObjCache;
	BBlockCache *theMallocCache;
	int numBlocksInCache;
	size_t sizeOfBlocksInCache;
	size_t sizeOfNonCacheBlocks;
	
	void *GetBlock(BBlockCache *theCache, size_t blockSize,
				   thread_id theThread, BList *cacheList, BList *nonCacheList);
	void SaveBlock(BBlockCache *theCache, void *, size_t blockSize,
	               thread_id theThread, BList *cacheList, BList *nonCacheList);
	void FreeBlock(void *, size_t blockSize, bool isMallocTest,
				   thread_id theThread, BList *cacheList,
				   BList *nonCacheList);
	void TestBlockCache(BBlockCache *theCache, bool isMallocTest);
	
public:
	static Test *suite(void);
	void TestThreadObj(void);
	void TestThreadMalloc(void);
	virtual void setUp(void);
	virtual void tearDown(void);
	BlockCacheConcurrencyTest(std::string);
	virtual ~BlockCacheConcurrencyTest();
};
	
#endif
