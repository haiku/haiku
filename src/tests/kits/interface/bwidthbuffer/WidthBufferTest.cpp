#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "WidthBufferTest.h"
#include "WidthBufferSimpleTest.h"


CppUnit::Test *WidthBufferTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(WidthBufferSimpleTest::suite());	
	
	return testSuite;
}
