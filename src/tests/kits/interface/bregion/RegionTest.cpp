#include "../common.h"
#include "RegionConstruction.h"
#include "RegionExclude.h"
#include "RegionInclude.h"
#include "RegionIntersect.h"
#include "RegionOffsetBy.h"

Test *RegionTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(RegionConstruction::suite());
	testSuite->addTest(RegionExclude::suite());
	testSuite->addTest(RegionInclude::suite());
	testSuite->addTest(RegionIntersect::suite());
	testSuite->addTest(RegionOffsetBy::suite());
	
	return(testSuite);
}







