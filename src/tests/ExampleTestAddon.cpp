#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "ExampleTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("ExampleSuite");
	suite->addTest("ExampleTests", ExampleTest::Suite());
	return suite;
}
