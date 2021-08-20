/*
	$Id: RegionOffsetBy.cpp 4235 2003-08-06 06:46:06Z jackburton $
	
	This file implements the OffsetBy test for the Haiku BRegion
	code.
	
	*/


#include "RegionOffsetBy.h"
#include <Region.h>
#include <Rect.h>

#include <assert.h>


/*
 *  Method:  RegionOffsetBy::RegionOffsetBy()
 *   Descr:  This is the constructor for this class.
 */		

RegionOffsetBy::RegionOffsetBy(std::string name) :
	RegionTestcase(name)
{
	}


/*
 *  Method:  RegionOffsetBy::~RegionOffsetBy()
 *   Descr:  This is the destructor for this class.
 */
 
RegionOffsetBy::~RegionOffsetBy()
{
	}


/*
 *  Method:  RegionOffsetBy::testOneRegion()
 *   Descr:  This member function performs a test on a single passed in
 *           region.
 */	

void RegionOffsetBy::testOneRegion(BRegion *testRegion)
{
	BRegion tempRegion1(*testRegion);
	CheckFrame(&tempRegion1);
	
	tempRegion1.OffsetBy(10.0, 20.0);
	if (RegionIsEmpty(testRegion)) {
		assert(RegionsAreEqual(&tempRegion1, testRegion));
	} else {
		assert(!RegionsAreEqual(&tempRegion1, testRegion));
	}
	
	BRegion tempRegion2;
	CheckFrame(&tempRegion2);
	assert(RegionIsEmpty(&tempRegion2));
	
	for(int i = testRegion->CountRects() - 1; i >= 0; i--) {
		BRect tempRect = testRegion->RectAt(i);
		tempRect.OffsetBy(10.0, 20.0);
		tempRegion2.Include(tempRect);
		CheckFrame(&tempRegion2);
		assert(!RegionIsEmpty(&tempRegion2));
	}
	assert(RegionsAreEqual(&tempRegion1, &tempRegion2));
	if (RegionIsEmpty(testRegion)) {
		assert(RegionsAreEqual(&tempRegion2, testRegion));
	} else {
		assert(!RegionsAreEqual(&tempRegion2, testRegion));
	}
	
	tempRegion1.OffsetBy(-10.0, -20.0);
	CheckFrame(&tempRegion1);
	if (RegionIsEmpty(testRegion)) {
		assert(RegionsAreEqual(&tempRegion1, testRegion));
		assert(RegionsAreEqual(&tempRegion1, &tempRegion2));
	} else {
		assert(RegionsAreEqual(&tempRegion1, testRegion));
		assert(!RegionsAreEqual(&tempRegion1, &tempRegion2));
	}
	
	tempRegion2.OffsetBy(-10.0, -20.0);
	CheckFrame(&tempRegion2);
	assert(RegionsAreEqual(&tempRegion1, testRegion));
	assert(RegionsAreEqual(&tempRegion2, testRegion));
	assert(RegionsAreEqual(&tempRegion1, &tempRegion2));
}


/*
 *  Method:  RegionOffsetBy::testTwoRegions()
 *   Descr:  This member function performs a test on two regions passed in.
 */	

void RegionOffsetBy::testTwoRegions(BRegion *testRegionA, BRegion *testRegionB)
{

}
	

/*
 *  Method:  RegionOffsetBy::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "RegionOffsetBy".
 */

 Test *RegionOffsetBy::suite(void)
{	
	typedef CppUnit::TestCaller<RegionOffsetBy>
		RegionOffsetByCaller;
		
	return(new RegionOffsetByCaller("BRegion::OffsetBy Test", &RegionOffsetBy::PerformTest));
	}
