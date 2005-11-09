#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "AllocatorTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("makeudfimage");
	suite->addTest("Udf::Allocator", AllocatorTest::Suite());
	return suite;
}
