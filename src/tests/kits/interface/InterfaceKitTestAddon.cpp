#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "bbitmap/BitmapTest.h"
#include "bdeskbar/DeskbarTest.h"
#include "bpolygon/PolygonTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Interface");

	// ##### Add test suites here #####
	suite->addTest("BBitmap", BitmapTestSuite());
	suite->addTest("BDeskbar", DeskbarTestSuite());
	suite->addTest("BPolygon", PolygonTestSuite());
	
	return suite;
}
