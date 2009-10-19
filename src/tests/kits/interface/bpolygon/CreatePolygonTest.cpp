/*
	$Id: CreatePolygonTest.cpp 2067 2002-11-23 04:42:57Z jrand $
	
	This file contains the implementation of the tests which show
	that BPolygons can be created successfully a number of 
	different ways.  The following use cases are tested:
		- Construction 1
		- Construction 2
		- Construction 3
		- Destruction
		- Add Points
		- Count Points
		- Frame
		- Assignment Operator
	
	*/


#include "CreatePolygonTest.h"
#include "DummyPolygon.h"
#include <Point.h>
#include <Rect.h>
#include <Polygon.h>


/*
 *  Method:  CreatePolygonTest::CreatePolygonTest()
 *   Descr:  This is the constructor for this class.
 */
		

	CreatePolygonTest::CreatePolygonTest(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  CreatePolygonTest::~CreatePolygonTest()
 *   Descr:  This is the destructor for this class.
 */
 

	CreatePolygonTest::~CreatePolygonTest()
{
	}
	
	
/*
 *  Method:  CreatePolygonTest::polygonMatches()
 *   Descr:  This member function compares the passed in polygon to the
 *           set of points and the expected frame rectangle.
 */
 
void CreatePolygonTest::polygonMatches(BPolygon *srcPoly, const BPoint *pointArray,
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
 *  Method:  CreatePolygonTest::PerformTest()
 *   Descr:  This member function tests the creation of BPolygon's.
 *
 *           The code does the following:
 *             - creates an array of points to test with
 *             - creates five polygons which should all be the same but in
 *               different ways:
 *                  1. By passing the point array in the constructor
 *                  2. By using the copy constructor from the first polygon
 *                  3. By using the AddPoints() member to add points to an
 *                     empty polygon
 *                  4. By using the assignment operator to replace an existing
 *                     polygon with a copy of the one from 1
 *                  5. By adding three points from the array on the constructor
 *                     and then adding th remaining points using the AddPoints()
 *                     member.
 *              - In each case, the polygonMatches() member is called to make
 *                sure the polygon is what is expected.
 */


	void CreatePolygonTest::PerformTest(void)
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
	
	BPolygon testPoly1(pointArray, numPoints);
	polygonMatches(&testPoly1, pointArray, numPoints, pointArrayFrame);
	
	BPolygon testPoly2(&testPoly1);
	polygonMatches(&testPoly2, pointArray, numPoints, pointArrayFrame);
	
	BPolygon testPoly3;
	testPoly3.AddPoints(pointArray, numPoints);
	polygonMatches(&testPoly3, pointArray, numPoints, pointArrayFrame);
	
	BPolygon testPoly4;
	testPoly4.AddPoints(&pointArray[2], 2);
	testPoly4 = testPoly1;
	polygonMatches(&testPoly4, pointArray, numPoints, pointArrayFrame);
	
	BPolygon testPoly5(pointArray, 3);
	testPoly5.AddPoints(&pointArray[3], numPoints - 3);
	polygonMatches(&testPoly5, pointArray, numPoints, pointArrayFrame);
}
	

/*
 *  Method:  PropertyConstructionTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "CreatePolygonTest".
 */

 Test *CreatePolygonTest::suite(void)
{	
	typedef CppUnit::TestCaller<CreatePolygonTest>
		CreatePolygonTestCaller;
		
	return(new CreatePolygonTestCaller("BPolygon::Create Polygon Test", &CreatePolygonTest::PerformTest));
	}
