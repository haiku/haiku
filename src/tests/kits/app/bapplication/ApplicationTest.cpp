#include "../common.h"
#include "AppRunTester.h"
#include "BApplicationTester.h"

CppUnit::Test* ApplicationTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(AppRunTester::Suite());
	testSuite->addTest(TBApplicationTester::Suite());

	return testSuite;
}

