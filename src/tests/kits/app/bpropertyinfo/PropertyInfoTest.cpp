#include "../common.h"
#include "PropertyConstructionTest.h"
#include "PropertyFindMatchTest.h"
#include "PropertyFlattenTest.h"

Test *PropertyInfoTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(PropertyConstructionTest::suite());
	testSuite->addTest(PropertyFindMatchTest::suite());
	testSuite->addTest(PropertyFlattenTest::suite());
	
	return(testSuite);
}







