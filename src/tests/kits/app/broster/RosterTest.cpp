#include "../common.h"
#include "BroadcastTester.h"
#include "FindAppTester.h"
#include "GetAppInfoTester.h"
#include "GetAppListTester.h"
#include "GetRecentTester.h"
#include "IsRunningTester.h"
#include "LaunchTester.h"
#include "RosterWatchingTester.h"
#include "TeamForTester.h"

CppUnit::Test* RosterTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(BroadcastTester::Suite());
	testSuite->addTest(FindAppTester::Suite());
	testSuite->addTest(GetAppInfoTester::Suite());
	testSuite->addTest(GetAppListTester::Suite());
	testSuite->addTest(GetRecentTester::Suite());
	testSuite->addTest(IsRunningTester::Suite());
	testSuite->addTest(LaunchTester::Suite());
	testSuite->addTest(RosterWatchingTester::Suite());
	testSuite->addTest(TeamForTester::Suite());

	return testSuite;
}

