#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "bbitmap/BitmapTest.h"
#include "bdeskbar/DeskbarTest.h"
#include "bpolygon/PolygonTest.h"
#include "bregion/RegionTest.h"
#include "bwidthbuffer/WidthBufferTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Interface");

	// ##### Add test suites here #####
	suite->addTest("BBitmap", BitmapTestSuite());
	suite->addTest("BDeskbar", DeskbarTestSuite());
	suite->addTest("BPolygon", PolygonTestSuite());
	suite->addTest("BRegion", RegionTestSuite());
	suite->addTest("_BWidthBuffer_", WidthBufferTestSuite());
	
	return suite;
}
