#include "../common.h"
#include "AppQuitRequestedTester.h"
#include "AppQuitTester.h"
#include "AppRunTester.h"
#include "BApplicationTester.h"

CppUnit::Test* ApplicationTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(AppQuitRequestedTester::Suite());
	testSuite->addTest(AppQuitTester::Suite());
	testSuite->addTest(AppRunTester::Suite());
	testSuite->addTest(TBApplicationTester::Suite());

	return testSuite;
}

