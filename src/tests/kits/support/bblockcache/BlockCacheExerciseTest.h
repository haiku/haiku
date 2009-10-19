/*
	$Id: BlockCacheExerciseTest.h 4522 2003-09-07 11:53:03Z bonefish $
	
	This file defines a class for performing tests on the BBlockCache class.
	
	*/


#ifndef BlockCacheExerciseTest_H
#define BlockCacheExerciseTest_H


#include "cppunit/TestCase.h"
#include <List.h>


class BBlockCache;

	
class BlockCacheExerciseTest : public CppUnit::TestCase {
	
private:
	BBlockCache *theCache;
	int numBlocksInCache;
	size_t sizeOfBlocksInCache;
	size_t sizeOfNonCacheBlocks;
	
	bool isMallocTest;
	
	BList freeList;
	BList usedList;
	BList nonCacheList;
	
	void BuildLists(void);
	void *GetBlock(size_t blockSize);
	void SaveBlock(void *, size_t blockSize);
	void FreeBlock(void *, size_t blockSize);
	void TestBlockCache(void);

protected:
	
public:
	static CppUnit::Test *suite(void);
	BlockCacheExerciseTest(std::string = "");
	virtual ~BlockCacheExerciseTest();
	virtual void PerformTest(void);
};
	
#endif
