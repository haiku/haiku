#include "../common.h"
#include "DeskbarGetItemTest.h"

Test *DeskbarTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(DeskbarGetItemTest::suite());
	
	return(testSuite);
}







