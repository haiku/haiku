#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "MemoryIOTest.h"
#include "ConstTest.h"
#include "SeekTest.h"
#include "WriteTest.h"
#include "ReadTest.h"
#include "SetSizeTest.h"

CppUnit::Test *MemoryIOTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(ConstTest::suite());
	testSuite->addTest(SeekTest::suite());
	testSuite->addTest(WriteTest::suite());
	testSuite->addTest(ReadTest::suite());
	testSuite->addTest(SetSizeTest::suite());
	
	return(testSuite);
}







