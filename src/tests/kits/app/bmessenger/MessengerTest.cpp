#include "../common.h"
#include "BMessengerTester.h"
#include "LockTargetTester.h"
#include "LockTargetWithTimeoutTester.h"
#include "MessengerAssignmentTester.h"
#include "MessengerComparissonTester.h"
#include "SendMessageTester.h"
#include "TargetTester.h"

CppUnit::Test* MessengerTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(LockTargetTester::Suite());
	testSuite->addTest(LockTargetWithTimeoutTester::Suite());
	testSuite->addTest(MessengerAssignmentTester::Suite());
	testSuite->addTest(MessengerComparissonTester::Suite());
	testSuite->addTest(SendMessageTester::Suite());
	testSuite->addTest(TBMessengerTester::Suite());
	testSuite->addTest(TargetTester::Suite());

	return testSuite;
}

