#include "BBitmapTester.h"
#include "SetBitsTester.h"

CppUnit::Test* BitmapTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(SetBitsTester::Suite());
	testSuite->addTest(TBBitmapTester::Suite());

	return testSuite;
}

