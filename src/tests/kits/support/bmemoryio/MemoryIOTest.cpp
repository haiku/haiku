#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "MemoryIOTest.h"
#include "ConstTest.h"
#include "SeekTest.h"
#include "WriteTest.h"

CppUnit::Test *MemoryIOTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(ConstTest::suite());
	testSuite->addTest(SeekTest::suite());
	testSuite->addTest(WriteTest::suite());
	
	return(testSuite);
}







