#include "../common.h"
#include "IsRunningTester.h"

CppUnit::Test* RosterTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(IsRunningTester::Suite());

	return testSuite;
}

