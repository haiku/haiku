/*
	$Id: BlockCacheTest.cpp 4522 2003-09-07 11:53:03Z bonefish $
*/
	
	
#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "BlockCacheExerciseTest.h"
#include "BlockCacheConcurrencyTest.h"


CppUnit::Test* BlockCacheTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(BlockCacheExerciseTest::suite());
	testSuite->addTest(BlockCacheConcurrencyTest::suite());
	
	return testSuite;
}

