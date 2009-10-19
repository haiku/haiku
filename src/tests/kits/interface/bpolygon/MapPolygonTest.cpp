/*
	$Id: MapPolygonTest.cpp 2136 2002-12-03 04:07:21Z jrand $
	
	This file contains the implementation of the tests which show
	that BPolygons can be created successfully a number of 
	different ways.  The following use cases are tested:
		- Map To
	
	*/


#include "MapPolygonTest.h"
#include "DummyPolygon.h"
#include <Point.h>
#include <Rect.h>
#include <Polygon.h>


/*
 *  Method:  MapPolygonTest::MapPolygonTest()
 *   Descr:  This is the constructor for this class.
 */
		

	MapPolygonTest::MapPolygonTest(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  MapPolygonTest::~MapPolygonTest()
 *   Descr:  This is the destructor for this class.
 */
 

	MapPolygonTest::~MapPolygonTest()
{
	}
	
	
/*
 *  Method:  MapPolygonTest::polygonMatches()
 *   Descr:  This member function compares the passed in polygon to the
 *           set of points and the expected frame rectangle.
 */
 
void MapPolygonTest::polygonMatches(BPolygon *srcPoly, const BPoint *pointArray,
                                       int numPoints, const BRect expectedFrame)
{
	assert(numPoints == srcPoly->CountPoints());
	assert(expectedFrame == srcPoly->Frame());
	
	const BPoint *srcPointArray = ((DummyPolygon *)srcPoly)->GetPoints();
	int i;
	for(i = 0; i < numPoints; i++) {
		assert(srcPointArray[i] == pointArray[i]);
	}
}


/*
 *  Method:  MapPolygonTest::PerformTest()
 *   Descr:  This member function tests the creation of BPolygon's.
 *
 *           The code does the following:
 */


	void MapPolygonTest::PerformTest(void)
{
	const int numPoints = 7;
	BPoint pointArray[numPoints];
	pointArray[0].x =  0.0;  pointArray[0].y = 10.0;
	pointArray[1].x = 10.0;  pointArray[1].y =  0.0;
	pointArray[2].x = 10.0;  pointArray[2].y =  5.0;
	pointArray[3].x = 30.0;  pointArray[3].y =  5.0;
	pointArray[4].x = 30.0;  pointArray[4].y = 15.0;
	pointArray[5].x = 10.0;  pointArray[5].y = 15.0;
	pointArray[6].x = 10.0;  pointArray[6].y = 20.0;
	BRect pointArrayFrame(0.0, 0.0, 30.0, 20.0);
	
	BRect fromRect(1.0, 1.0, 3.0, 3.0);
	BRect toRect(1.0, 4.0, 5.0, 10.0);
	
	BPoint mapPointArray[numPoints];
	mapPointArray[0].x = -1.0;  mapPointArray[0].y = 31.0;
	mapPointArray[1].x = 19.0;  mapPointArray[1].y =  1.0;
	mapPointArray[2].x = 19.0;  mapPointArray[2].y = 16.0;
	mapPointArray[3].x = 59.0;  mapPointArray[3].y = 16.0;
	mapPointArray[4].x = 59.0;  mapPointArray[4].y = 46.0;
	mapPointArray[5].x = 19.0;  mapPointArray[5].y = 46.0;
	mapPointArray[6].x = 19.0;  mapPointArray[6].y = 61.0;
	BRect mapPointArrayFrame(-1.0, 1.0, 59.0, 61.0);
	
	BPolygon testPoly1(pointArray, numPoints);
	polygonMatches(&testPoly1, pointArray, numPoints, pointArrayFrame);
	testPoly1.MapTo(fromRect, toRect);
	polygonMatches(&testPoly1, mapPointArray, numPoints, mapPointArrayFrame);
}
	

/*
 *  Method:  PropertyConstructionTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "MapPolygonTest".
 */

 Test *MapPolygonTest::suite(void)
{	
	typedef CppUnit::TestCaller<MapPolygonTest>
		MapPolygonTestCaller;
		
	return(new MapPolygonTestCaller("BPolygon::Map Polygon Test", &MapPolygonTest::PerformTest));
	}
