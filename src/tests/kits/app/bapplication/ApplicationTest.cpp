#include "../common.h"
#include "BApplicationTester.h"

CppUnit::Test* ApplicationTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(TBApplicationTester::Suite());

	return testSuite;
}

