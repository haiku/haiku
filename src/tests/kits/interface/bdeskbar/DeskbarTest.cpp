#include "../common.h"
#include "DeskbarGetItemTest.h"
#include "DeskbarLocationTest.h"
#include "DeskbarAddItemTest.h"

Test *DeskbarTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(DeskbarGetItemTest::suite());
	testSuite->addTest(DeskbarLocationTest::suite());
	testSuite->addTest(DeskbarAddItemTest::suite());
	
	return(testSuite);
}
