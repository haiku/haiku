#include "../common.h"
#include "BMessengerTester.h"
#include "LockTargetTester.h"
#include "TargetTester.h"

CppUnit::Test* MessengerTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(LockTargetTester::Suite());
	testSuite->addTest(TBMessengerTester::Suite());
	testSuite->addTest(TargetTester::Suite());
	
	return testSuite;
}
