#include "../common.h"
#include "BCursorTester.h"

CppUnit::Test* CursorTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(BCursorTester::Suite());

	return testSuite;
}

