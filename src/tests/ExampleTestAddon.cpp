#include <TestSuite.h>
#include <TestSuiteAddon.h>
#include <ExampleTest.h>

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Example");
	suite->addTest("BExample", ExampleTest::Suite());
	return suite;
}
