#include "../common.h"
#include "BMessageRunnerTester.h"
#include "GetInfoTester.h"
#include "SetCountTester.h"
#include "SetIntervalTester.h"

CppUnit::Test* MessageRunnerTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(GetInfoTester::Suite());
	testSuite->addTest(SetCountTester::Suite());
	testSuite->addTest(SetIntervalTester::Suite());
	testSuite->addTest(TBMessageRunnerTester::Suite());

	return testSuite;
}

