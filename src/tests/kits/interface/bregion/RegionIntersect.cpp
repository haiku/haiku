/*
	$Id: RegionIntersect.cpp 4235 2003-08-06 06:46:06Z jackburton $
	
	This file implements the intersect test for the Haiku BRegion
	code.
	
	*/


#include "RegionIntersect.h"
#include <Region.h>
#include <Rect.h>

#include <assert.h>


/*
 *  Method:  RegionIntersect::RegionIntersect()
 *   Descr:  This is the constructor for this class.
 */		

RegionIntersect::RegionIntersect(std::string name) :
	RegionTestcase(name)
{
	}


/*
 *  Method:  RegionIntersect::~RegionIntersect()
 *   Descr:  This is the destructor for this class.
 */
 
RegionIntersect::~RegionIntersect()
{
	}


/*
 *  Method:  RegionExclude::CheckIntersect()
 *   Descr:  This member function checks that the result region is in fact
 *           region A with region B intersected.
 */	

void RegionIntersect::CheckIntersect(BRegion *resultRegion, BRegion *testRegionA,
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
		tempRegion.Include(testRegionB);
		assert(RegionsAreEqual(&tempRegion, testRegionB));
	}
	
	if (RegionsAreEqual(resultRegion, testRegionB)) {
		BRegion tempRegion(*testRegionA);
		tempRegion.Include(testRegionB);
		assert(RegionsAreEqual(&tempRegion, testRegionA));
	}
	
	if ((!resultRegionEmpty) && (!testRegionAEmpty) && (!testRegionBEmpty)) {
		BRect resultRegionFrame(resultRegion->Frame());
		BRect testRegionAFrame(testRegionA->Frame());
		BRect testRegionBFrame(testRegionB->Frame());
		BRect tempRect = (testRegionAFrame & testRegionBFrame);
		
		assert(tempRect.Contains(resultRegionFrame));
	}
	
	if (testRegionBEmpty) {
		assert(resultRegionEmpty);
	} else {
		for(int i = 0; i < testRegionB->CountRects(); i++) {
			BRect tempRect = testRegionB->RectAt(i);
			
			assert(testRegionB->Intersects(tempRect));
			if (testRegionA->Intersects(tempRect)) {
				assert(resultRegion->Intersects(tempRect));
			} else {
				assert(!resultRegion->Intersects(tempRect));
			}
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(testRegionB->Contains(pointArray[j]));
				if (testRegionA->Contains(pointArray[j])) {
					assert(resultRegion->Contains(pointArray[j]));
				} else {
					assert(!resultRegion->Contains(pointArray[j]));
				}
			}
		}
	}
	
	if (testRegionAEmpty) {
		assert(resultRegionEmpty);
	} else {
		for(int i = 0; i < testRegionA->CountRects(); i++) {
			BRect tempRect = testRegionA->RectAt(i);
			
			assert(testRegionA->Intersects(tempRect));
			if (testRegionB->Intersects(tempRect)) {
				assert(resultRegion->Intersects(tempRect));
			} else {
				assert(!resultRegion->Intersects(tempRect));
			}
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(testRegionA->Contains(pointArray[j]));
				if (testRegionB->Contains(pointArray[j])) {
					assert(resultRegion->Contains(pointArray[j]));
				} else {
					assert(!resultRegion->Contains(pointArray[j]));
				}
			}
		}
	}

	if (resultRegionEmpty) {
		BRegion tempRegion(*testRegionA);
		tempRegion.Exclude(testRegionB);
		assert(RegionsAreEqual(&tempRegion, testRegionA));
		
		tempRegion = *testRegionB;
		tempRegion.Exclude(testRegionA);
		assert(RegionsAreEqual(&tempRegion, testRegionB));
	} else {
		for(int i = 0; i < resultRegion->CountRects(); i++) {
			BRect tempRect = resultRegion->RectAt(i);
			
			assert(resultRegion->Intersects(tempRect));
			assert(testRegionA->Intersects(tempRect));
			assert(testRegionB->Intersects(tempRect));
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(resultRegion->Contains(pointArray[j]));
				assert(testRegionA->Contains(pointArray[j]));
				assert(testRegionB->Contains(pointArray[j]));
			}
		}
	}
}


/*
 *  Method:  RegionIntersect::testOneRegion()
 *   Descr:  This member function performs a test on a single passed in
 *           region.
 */	

void RegionIntersect::testOneRegion(BRegion *testRegion)
{

}


/*
 *  Method:  RegionIntersect::testTwoRegions()
 *   Descr:  This member function performs a test on two regions passed in.
 */	

void RegionIntersect::testTwoRegions(BRegion *testRegionA, BRegion *testRegionB)
{
	BRegion tempRegion1(*testRegionA);
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionA));
	
	tempRegion1.IntersectWith(testRegionB);
	CheckFrame(&tempRegion1);
	CheckIntersect(&tempRegion1, testRegionA, testRegionB);
}
	

/*
 *  Method:  RegionIntersect::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "RegionIntersect".
 */

 Test *RegionIntersect::suite(void)
{	
	typedef CppUnit::TestCaller<RegionIntersect>
		RegionIntersectCaller;
		
	return(new RegionIntersectCaller("BRegion::Intersect Test", &RegionIntersect::PerformTest));
	}
