#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "bbitmap/BitmapTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Interface");

	// ##### Add test suites here #####
	suite->addTest("BBitmap", BitmapTestSuite());
	
	return suite;
}
