/*
	$Id: RegionExclude.cpp 4235 2003-08-06 06:46:06Z jackburton $
	
	This file implements the exclude test for the Haiku BRegion
	code.
	
	*/


#include "RegionExclude.h"
#include <Region.h>
#include <Rect.h>

#include <assert.h>


/*
 *  Method:  RegionExclude::RegionExclude()
 *   Descr:  This is the constructor for this class.
 */		

RegionExclude::RegionExclude(std::string name) :
	RegionTestcase(name)
{
	}


/*
 *  Method:  RegionExclude::~RegionExclude()
 *   Descr:  This is the destructor for this class.
 */
 
RegionExclude::~RegionExclude()
{
	}


/*
 *  Method:  RegionExclude::CheckExclude()
 *   Descr:  This member function checks that the result region is in fact
 *           region A with region B excluded.
 */	

void RegionExclude::CheckExclude(BRegion *resultRegion, BRegion *testRegionA,
                                 BRegion *testRegionB)
{
	bool resultRegionEmpty = RegionIsEmpty(resultRegion);
	bool testRegionAEmpty = RegionIsEmpty(testRegionA);
	bool testRegionBEmpty = RegionIsEmpty(testRegionB);
	
	if (RegionsAreEqual(testRegionA, testRegionB)) {
		assert(resultRegionEmpty);
	}
	
	if (RegionsAreEqual(resultRegion, testRegionA)) {
		BRegion tempRegion(*testRegionA);
		tempRegion.IntersectWith(testRegionB);
		assert(RegionIsEmpty(&tempRegion));
	}
	
	if (RegionsAreEqual(resultRegion, testRegionB)) {
		assert(resultRegionEmpty);
		assert(testRegionAEmpty);
		assert(testRegionBEmpty);
	}
	
	if ((!resultRegionEmpty) && (!testRegionAEmpty) && (!testRegionBEmpty)) {
		BRect resultRegionFrame(resultRegion->Frame());
		BRect testRegionAFrame(testRegionA->Frame());
		BRect testRegionBFrame(testRegionB->Frame());
			
		if (!testRegionAFrame.Intersects(testRegionBFrame)) {
			assert(testRegionAFrame == resultRegionFrame);
			assert(RegionsAreEqual(resultRegion, testRegionA));
		} else {
			assert(testRegionAFrame.Contains(resultRegionFrame));
		}
	}
	
	if (testRegionBEmpty) {
		assert(RegionsAreEqual(resultRegion, testRegionA));
	} else {
		assert(!RegionsAreEqual(resultRegion, testRegionB));
		for(int i = 0; i < testRegionB->CountRects(); i++) {
			BRect tempRect = testRegionB->RectAt(i);
			
			assert(testRegionB->Intersects(tempRect));
			assert(!resultRegion->Intersects(tempRect));
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(testRegionB->Contains(pointArray[j]));
				assert(!resultRegion->Contains(pointArray[j]));
			}
		}
	}
	
	if (testRegionAEmpty) {
		assert(resultRegionEmpty);
	} else {
		for(int i = 0; i < testRegionA->CountRects(); i++) {
			BRect tempRect = testRegionA->RectAt(i);
			
			assert(testRegionA->Intersects(tempRect));
			if (!testRegionB->Intersects(tempRect)) {
				assert(resultRegion->Intersects(tempRect));
			}
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(testRegionA->Contains(pointArray[j]));
				if (testRegionB->Contains(pointArray[j])) {
					assert(!resultRegion->Contains(pointArray[j]));
				} else {
					assert(resultRegion->Contains(pointArray[j]));
				}
			}
		}
	}

	if (resultRegionEmpty) {
		BRegion tempRegion(*testRegionA);
		tempRegion.IntersectWith(testRegionB);
		assert(RegionsAreEqual(&tempRegion, testRegionA));
	} else {
		assert(!RegionsAreEqual(resultRegion, testRegionB));
		for(int i = 0; i < resultRegion->CountRects(); i++) {
			BRect tempRect = resultRegion->RectAt(i);
			
			assert(resultRegion->Intersects(tempRect));
			assert(testRegionA->Intersects(tempRect));
			assert(!testRegionB->Intersects(tempRect));
			
			BPoint *pointArray;
			int numPoints = GetPointsInRect(tempRect, &pointArray);
			for (int j = 0; j < numPoints; j++) {
				assert(resultRegion->Contains(pointArray[j]));
				assert(testRegionA->Contains(pointArray[j]));
				assert(!testRegionB->Contains(pointArray[j]));
			}
		}
	}
}


/*
 *  Method:  RegionExclude::testOneRegion()
 *   Descr:  This member function performs a test on a single passed in
 *           region.
 */	

void RegionExclude::testOneRegion(BRegion *testRegion)
{

}


/*
 *  Method:  RegionExclude::testTwoRegions()
 *   Descr:  This member function performs a test on two regions passed in.
 */	

void RegionExclude::testTwoRegions(BRegion *testRegionA, BRegion *testRegionB)
{
	BRegion tempRegion1(*testRegionA);
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionA));
	
	tempRegion1.Exclude(testRegionB);
	CheckFrame(&tempRegion1);
	CheckExclude(&tempRegion1, testRegionA, testRegionB);
	
	tempRegion1 = *testRegionA;
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionA));
	
	for(int i = 0; i < testRegionB->CountRects(); i++) {
		tempRegion1.Exclude(testRegionB->RectAt(i));
		CheckFrame(&tempRegion1);
	}
	CheckExclude(&tempRegion1, testRegionA, testRegionB);
}
	

/*
 *  Method:  RegionExclude::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "RegionExclude".
 */

 Test *RegionExclude::suite(void)
{	
	typedef CppUnit::TestCaller<RegionExclude>
		RegionExcludeCaller;
		
	return(new RegionExcludeCaller("BRegion::Exclude Test", &RegionExclude::PerformTest));
	}
