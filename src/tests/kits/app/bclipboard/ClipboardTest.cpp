#include "../common.h"
#include "BClipboardTester.h"
#include "CountTester.h"
#include "LockTester.h"
#include "ReadWriteTester.h"

CppUnit::Test* ClipboardTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(BClipboardTester::Suite());
	testSuite->addTest(CountTester::Suite());
	testSuite->addTest(LockTester::Suite());
	testSuite->addTest(ReadWriteTester::Suite());

	return testSuite;
}

