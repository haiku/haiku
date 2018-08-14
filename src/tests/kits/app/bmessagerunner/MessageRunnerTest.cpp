#include "../common.h"
#include "BMessageRunnerTester.h"
#include "GetInfoTester.h"
#include "SetCountTester.h"
#include "SetIntervalTester.h"

CppUnit::Test* MessageRunnerTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	// TODO: These tests deadlock in ~MessageRunnerTestApp on Lock() call
	//testSuite->addTest(GetInfoTester::Suite());
	//testSuite->addTest(SetCountTester::Suite());
	//testSuite->addTest(SetIntervalTester::Suite());
	//testSuite->addTest(TBMessageRunnerTester::Suite());

	return testSuite;
}

