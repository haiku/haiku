#include "../common.h"
#include "DeskbarGetItemTest.h"
#include "DeskbarLocationTest.h"

Test *DeskbarTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(DeskbarGetItemTest::suite());
	testSuite->addTest(DeskbarLocationTest::suite());
	
	return(testSuite);
}







