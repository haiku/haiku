/*
	$Id: RegionInclude.cpp 4235 2003-08-06 06:46:06Z jackburton $
	
	This file implements the include test for the Haiku BRegion
	code.
	
	*/


#include "RegionInclude.h"
#include <Region.h>
#include <Rect.h>

#include <assert.h>


/*
 *  Method:  RegionInclude::RegionInclude()
 *   Descr:  This is the constructor for this class.
 */		

RegionInclude::RegionInclude(std::string name) :
	RegionTestcase(name)
{
	}


/*
 *  Method:  RegionInclude::~RegionInclude()
 *   Descr:  This is the destructor for this class.
 */
 
RegionInclude::~RegionInclude()
{
	}


/*
 *  Method:  RegionExclude::CheckInclude()
 *   Descr:  This member function checks that the result region is in fact
 *           region A with region B included.
 */	

void RegionInclude::CheckInclude(BRegion *resultRegion, BRegion *testRegionA,
                                 BRegion *testRegionB)
{
	bool resultRegionEmpty = RegionIsEmpty(resultRegion);
	bool testRegionAEmpty = RegionIsEmpty(testRegionA);
	bool testRegionBEmpty = RegionIsEmpty(testRegionB);
	
	if (RegionsAreEqual(testRegionA, testRegionB)) {
		assert(RegionsAreEqual(resultRegion, testRegionA));
	}
	
	if (RegionsAreEqual(resultRegion, testRegionA)) {
		BRegion tempRegion(*testRegionA);
		tempRegion.IntersectWith(testRegionB);
		assert(RegionsAreEqual(&tempRegion, testRegionB));
	}
	
	if (RegionsAreEqual(resultRegion, testRegionB)) {
		BRegion tempRegion(*testRegionA);
		tempRegion.IntersectWith(testRegionB);
		assert(RegionsAreEqual(&tempRegion, testRegionA));
	}
	
	if ((!resultRegionEmpty) && (!testRegionAEmpty) && (!testRegionBEmpty)) {
		BRect resultRegionFrame(resultRegion->Frame());
		BRect testRegionAFrame(testRegionA->Frame());
		BRect testRegionBFrame(testRegionB->Frame());
		
		assert(resultRegionFrame == (testRegionAFrame | testRegionBFrame));
	}
	
	if (testRegionBEmpty) {
		assert(RegionsAreEqual(resultRegion, testRegionA));
	} else {
		for(int i = 0; i < testRegionB->CountRects(); i++) {
			BRect tempRect = testRegionB->RectAt(i);
			
			assert(testRegionB->Intersects(tempRect));
			assert(resultRegion->Intersects(tempRect));
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(testRegionB->Contains(pointArray[j]));
				assert(resultRegion->Contains(pointArray[j]));
			}
		}
	}
	
	if (testRegionAEmpty) {
		assert(RegionsAreEqual(resultRegion, testRegionB));
	} else {
		for(int i = 0; i < testRegionA->CountRects(); i++) {
			BRect tempRect = testRegionA->RectAt(i);
			
			assert(testRegionA->Intersects(tempRect));
			assert(resultRegion->Intersects(tempRect));
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(testRegionA->Contains(pointArray[j]));
				assert(resultRegion->Contains(pointArray[j]));
			}
		}
	}

	if (resultRegionEmpty) {
		assert(testRegionAEmpty);
		assert(testRegionBEmpty);
	} else {
		for(int i = 0; i < resultRegion->CountRects(); i++) {
			BRect tempRect = resultRegion->RectAt(i);
			
			assert(resultRegion->Intersects(tempRect));
			assert((testRegionA->Intersects(tempRect)) || 
			       (testRegionB->Intersects(tempRect)));
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(resultRegion->Contains(pointArray[j]));
				assert((testRegionA->Contains(pointArray[j])) ||
				       (testRegionB->Contains(pointArray[j])));
			}
		}
	}
}


/*
 *  Method:  RegionInclude::testOneRegion()
 *   Descr:  This member function performs a test on a single passed in
 *           region.
 */	

void RegionInclude::testOneRegion(BRegion *testRegion)
{

}


/*
 *  Method:  RegionInclude::testTwoRegions()
 *   Descr:  This member function performs a test on two regions passed in.
 */	

void RegionInclude::testTwoRegions(BRegion *testRegionA, BRegion *testRegionB)
{
	BRegion tempRegion1(*testRegionA);
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionA));
	
	tempRegion1.Include(testRegionB);
	CheckFrame(&tempRegion1);
	CheckInclude(&tempRegion1, testRegionA, testRegionB);
	
	tempRegion1 = *testRegionA;
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionA));
	
	for(int i = 0; i < testRegionB->CountRects(); i++) {
		tempRegion1.Include(testRegionB->RectAt(i));
		CheckFrame(&tempRegion1);
	}
	CheckInclude(&tempRegion1, testRegionA, testRegionB);
}
	

/*
 *  Method:  RegionInclude::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "RegionInclude".
 */

 Test *RegionInclude::suite(void)
{	
	typedef CppUnit::TestCaller<RegionInclude>
		RegionIncludeCaller;
		
	return(new RegionIncludeCaller("BRegion::Include Test", &RegionInclude::PerformTest));
	}
