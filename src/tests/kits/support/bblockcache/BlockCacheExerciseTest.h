/*
	$Id: BlockCacheExerciseTest.h,v 1.1 2003/09/07 11:53:03 bonefish Exp $
	
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
