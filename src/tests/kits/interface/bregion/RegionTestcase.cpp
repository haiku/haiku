/*
	$Id: RegionTestcase.cpp 7533 2004-05-12 06:37:18Z jackburton $
	
	This file implements a base class for all tests of the Haiku
	BRegion code.
	
	*/


#include "RegionTestcase.h"
#include <Region.h>
#include <Rect.h>

#include <assert.h>
#include <math.h>


/*
 *  Method:  RegionTestcase::RegionTestcase()
 *   Descr:  This is the constructor for this class.
 */
		
RegionTestcase::RegionTestcase(std::string name) :
	TestCase(name)
{
	const int numRectsPerRegion = 3;
	
	float theRegions[][numRectsPerRegion][4] = 
		{
			{
				{10.0, 10.0, 50.0, 50.0},
				{25.0, 10.0, 75.0, 40.0},
				{70.0, 100.0, 90.0, 120.0}
			},
			{
				{15.0, 15.0, 45.0, 45.0},
				{30.0, 15.0, 70.0, 35.0},
				{75.0, 105.0, 85.0, 115.0}
			},
			{
				{15.0, 15.0, 55.0, 55.0},
				{30.0, 15.0, 80.0, 45.0},
				{75.0, 105.0, 95.0, 125.0}
			},
			{
				{210.0, 210.0, 250.0, 250.0},
				{225.0, 210.0, 275.0, 240.0},
				{270.0, 300.0, 290.0, 320.0}
			},
			{
				{-50.0, -50.0, -10.0, -10.0},
				{-75.0, -40.0, -25.0, -10.0},
				{-90.0, -120.0, -70.0, -100.0}
			},
			{
				{-50.0, -30.0, 100.0, 120.0},
				{-140.0, 120.0, 340.0, 905.0},
				{-90.0, -120.0, 90.0, 120.0}
			},
			{
				// A test to see if our BRegion can handle non integers 
				// rects coordinates (as R5 do)
				{10.3, 10.8, 50.2, 50.9},
				{25.1, 10.7, 75.8, 40.4},
				{70.6, 100.6, 90.1, 120.3}
			},
			{
				{15.0, 25.0, 75.0, 80.0},
				{30.0, 15.0, 80.0, 45.0},
				{60.0, 20.0, 95.0, 225.0}
			},
			// These two regions were added to test our implementation
			// against the "Pahtz matrix test" (see interface kit
			// mailing list archives (April 2004) for more info)
			{
				{0.0, 50.0, 47.0, 50.0},
				{99.0, 50.0, 399.0, 50.0},
				{-100.0, -20.0, 100.0, -20.0}
			},
			{
				{0.0, 50.0, 47.0, 50.0},
				{100.0, 50.0, 399.0, 50.0},
				{-100.0, -20.0, 100.0, -20.0}
			}
		};
	
	const int numTestRegions = sizeof(theRegions) / sizeof(theRegions[0]);
		
	listOfRegions.AddItem(new BRegion);
	for(int regionNum = 0; regionNum < numTestRegions; regionNum++) {
		BRegion *tempRegion = new BRegion;
		for(int rectNum = 0; rectNum < numRectsPerRegion; rectNum++) {
			tempRegion->Include(BRect(theRegions[regionNum][rectNum][0],
			                          theRegions[regionNum][rectNum][1],
			                          theRegions[regionNum][rectNum][2],
			                          theRegions[regionNum][rectNum][3]));
		}
		listOfRegions.AddItem(tempRegion);
	}
}


/*
 *  Method:  RegionTestcase::~RegionTestcase()
 *   Descr:  This is the destructor for this class.
 */
 
RegionTestcase::~RegionTestcase()
{
	while(!listOfRegions.IsEmpty()) {
		delete static_cast<BRegion *>(listOfRegions.RemoveItem(int32(0)));
	}
}


/*
 *  Method:  RegionTestcase::GetPointsInRect()
 *   Descr:  This member function returns an array of BPoints on the edge and
 *           inside the passed in BRect.  It also returns the number of points
 *           in the array.
 */	

int RegionTestcase::GetPointsInRect(BRect theRect, BPoint **pointArrayPtr)
{
	*pointArrayPtr = pointArray;
	if (!theRect.IsValid()) {
		return(0);
	}
	
	float xIncrement = (theRect.Width() + 1.0) / (numPointsPerSide - 1);
	float yIncrement = (theRect.Height() + 1.0) / (numPointsPerSide - 1);
	
	int numPoints = 0;
	
	for(int i = 0; i < numPointsPerSide; i++) {
		float xCoord = theRect.left + (i * xIncrement);
		if (i == numPointsPerSide - 1) {
			xCoord = theRect.right;
		}
		for(int j = 0; j < numPointsPerSide; j++) {
			float yCoord = theRect.top + (j * yIncrement);
			if (j == numPointsPerSide - 1) {
				yCoord = theRect.bottom;
			}
			pointArray[numPoints].Set(floor(xCoord), floor(yCoord));
			assert(theRect.Contains(pointArray[numPoints]));
			numPoints++;
		}
	}
	return(numPoints); 
}


/*
 *  Method:  RegionTestcase::CheckFrame()
 *   Descr:  This member function checks that the BRegion's frame matches
 *           the regions contents.
 */	

void RegionTestcase::CheckFrame(BRegion *theRegion)
{
	BRect theFrame = theRegion->Frame();
	if (theFrame.IsValid()) {
		assert(!RegionIsEmpty(theRegion));

		BRect testFrame = theRegion->RectAt(0);
		assert(theFrame.Contains(testFrame));
		
		for(int i = 1; i < theRegion->CountRects(); i++) {
			BRect tempRect = theRegion->RectAt(i);
			assert(theFrame.Contains(tempRect));
			testFrame = testFrame | tempRect;
		}
		assert(testFrame == theFrame);	
	} else {
		assert(RegionIsEmpty(theRegion));
	}
}


/*
 *  Method:  RegionTestcase::RegionsAreEqual()
 *   Descr:  This member function returns true if the two BRegion's passed
 *           in are the same, otherwise it returns false.
 */	

bool RegionTestcase::RegionsAreEqual(BRegion *regionA, BRegion *regionB)
{
	bool result = false;
	
	if (regionA->CountRects() == regionB->CountRects()) {
		bool gotAMatch = true;
		for(int i = 0; i < regionA->CountRects(); i++) {
			gotAMatch = false;
			for(int j = 0; j < regionB->CountRects(); j++) {
				if (regionA->RectAt(i) == regionB->RectAt(j)) {
					gotAMatch = true;
					break;
				}
			}
			if (!gotAMatch) {
				break;
			}
		}
		if (gotAMatch) {
			result = true;
		}
	}
	
	if (!result) {
		BRegion tempRegion(*regionA);
		
		tempRegion.Exclude(regionB);
		if (RegionIsEmpty(&tempRegion)) {
			tempRegion = *regionB;
			tempRegion.Exclude(regionA);
			if (RegionIsEmpty(&tempRegion)) {
				result = true;
			}
		}
	}
			
	if (result) {
		assert(regionA->Frame() == regionB->Frame());
		if (regionA->CountRects() == 0) {
			assert(RegionIsEmpty(regionA));
			assert(RegionIsEmpty(regionB));
		}
	}
	return(result);
}


/*
 *  Method:  RegionTestcase::RegionsIsEmpty()
 *   Descr:  This member function returns true if the BRegion passed
 *           in is an empty region, otherwise it returns false.
 */	

bool RegionTestcase::RegionIsEmpty(BRegion *theRegion)
{
	if (theRegion->CountRects() == 0) {
		assert(!theRegion->Frame().IsValid());
		return(true);
	}
	assert(theRegion->Frame().IsValid());
	return(false);
}
	
	
/*
 *  Method:  RegionTestcase::PerformTest()
 *   Descr:  This member function iterates over the set of BRegion's for
 *           testing and calls testOneRegion() for each region.  Then it
 *           calls testTwoRegions() for each pair of regions (including
 *           when the two regions are the same).
 */

void RegionTestcase::PerformTest(void)
{
	int numItems = listOfRegions.CountItems();
	
	for(int i = 0; i < numItems; i++) {
		BRegion *testRegion = static_cast<BRegion *>(listOfRegions.ItemAt(i));
		
		CheckFrame(testRegion);
		testOneRegion(testRegion);
	}
	for(int i = 0; i < numItems; i++) {
		BRegion *testRegionA = static_cast<BRegion *>(listOfRegions.ItemAt(i));
		
		for(int j = 0; j < numItems; j++) {
			BRegion *testRegionB = static_cast<BRegion *>(listOfRegions.ItemAt(j));
			
			testTwoRegions(testRegionA, testRegionB);
		}
	}
}
