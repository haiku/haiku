#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "MallocIOTest.h"
#include "MallocSeekTest.h"
#include "MallocWriteTest.h"
#include "MallocBufferLengthTest.h"

CppUnit::Test *MallocIOTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(MallocSeekTest::suite());
	testSuite->addTest(MallocWriteTest::suite());
	testSuite->addTest(MallocBufferLengthTest::suite());
	
	return(testSuite);
}







