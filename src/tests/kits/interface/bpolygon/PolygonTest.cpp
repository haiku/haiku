#include "../common.h"
#include "CreatePolygonTest.h"
#include "MapPolygonTest.h"

Test *PolygonTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(CreatePolygonTest::suite());
	testSuite->addTest(MapPolygonTest::suite());
	
	return(testSuite);
}
