#include "../common.h"
#include "CreatePolygonTest.h"

Test *PolygonTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(CreatePolygonTest::suite());
	
	return(testSuite);
}
