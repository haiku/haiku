#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "blocker/LockerTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Support");

	// ##### Add test suites here #####
	suite->addTest("BLocker", LockerTestSuite());
	
	return suite;
}
