#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "StringTest.h"
#include "StringConstructionTest.h"
#include "StringCountCharTest.h"
#include "StringOperatorsTest.h"

CppUnit::Test *StringTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(StringConstructionTest::suite());
	testSuite->addTest(StringCountCharTest::suite());
	testSuite->addTest(StringOperatorsTest::suite());
	
	return(testSuite);
}
