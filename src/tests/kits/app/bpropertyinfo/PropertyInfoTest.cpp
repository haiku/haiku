#include "../common.h"
#include "PropertyConstructionTest1.h"

Test *PropertyInfoTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(PropertyConstructionTest1::suite());
	
	return(testSuite);
}







