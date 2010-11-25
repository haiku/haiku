#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "balert/AlertTest.h"
#include "bbitmap/BitmapTest.h"
#include "bdeskbar/DeskbarTest.h"
#include "bpolygon/PolygonTest.h"
#include "bregion/RegionTest.h"
//#include "bwidthbuffer/WidthBufferTest.h"
#include "GraphicsDefsTest.h"


BTestSuite *
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("Interface");

	// ##### Add test suites here #####
	suite->addTest("BAlert", AlertTest::Suite());
	suite->addTest("BBitmap", BitmapTestSuite());
	suite->addTest("BDeskbar", DeskbarTestSuite());
	suite->addTest("BPolygon", PolygonTestSuite());
	suite->addTest("BRegion", RegionTestSuite());
	//suite->addTest("_BWidthBuffer_", WidthBufferTestSuite());
	suite->addTest("GraphicsDefs", GraphicsDefsTestSuite());

	return suite;
}
