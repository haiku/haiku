#include "../common.h"
#include "FindAppTester.h"
#include "GetAppInfoTester.h"
#include "GetAppListTester.h"
#include "IsRunningTester.h"
#include "TeamForTester.h"

CppUnit::Test* RosterTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(FindAppTester::Suite());
	testSuite->addTest(GetAppInfoTester::Suite());
	testSuite->addTest(GetAppListTester::Suite());
	testSuite->addTest(IsRunningTester::Suite());
	testSuite->addTest(TeamForTester::Suite());

	return testSuite;
}

