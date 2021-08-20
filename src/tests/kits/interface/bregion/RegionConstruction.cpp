/*
	$Id: RegionConstruction.cpp 4235 2003-08-06 06:46:06Z jackburton $
	
	This file implements the construction test for the Haiku BRegion
	code.
	
	*/


#include "RegionConstruction.h"
#include <Region.h>
#include <Rect.h>

#include <assert.h>


/*
 *  Method:  RegionConstruction::RegionConstruction()
 *   Descr:  This is the constructor for this class.
 */		

RegionConstruction::RegionConstruction(std::string name) :
	RegionTestcase(name)
{
	}


/*
 *  Method:  RegionConstruction::~RegionConstruction()
 *   Descr:  This is the destructor for this class.
 */
 
RegionConstruction::~RegionConstruction()
{
	}


/*
 *  Method:  RegionConstruction::testOneRegion()
 *   Descr:  This member function performs a test on a single passed in
 *           region.
 */	

void RegionConstruction::testOneRegion(BRegion *testRegion)
{
	assert(!testRegion->RectAt(-1).IsValid());
	assert(!testRegion->RectAt(testRegion->CountRects()).IsValid());

	BRegion tempRegion1(*testRegion);
	assert(RegionsAreEqual(&tempRegion1, testRegion));
	CheckFrame(&tempRegion1);
	
	tempRegion1.MakeEmpty();
	assert(RegionIsEmpty(&tempRegion1));
	CheckFrame(&tempRegion1);
	
	if (!RegionIsEmpty(testRegion)) {
		assert(!RegionsAreEqual(&tempRegion1, testRegion));
		for(int i = testRegion->CountRects() - 1; i >= 0; i--) {
			tempRegion1.Include(testRegion->RectAt(i));
			CheckFrame(&tempRegion1);
		}
	}
	assert(RegionsAreEqual(&tempRegion1, testRegion));
	
	if (!RegionIsEmpty(testRegion)) {
		BRegion tempRegion2(testRegion->RectAt(0));
		CheckFrame(&tempRegion2);
		assert(!RegionIsEmpty(&tempRegion2));
		
		BRegion tempRegion3;
		CheckFrame(&tempRegion3);
		assert(RegionIsEmpty(&tempRegion3));
		tempRegion3.Set(testRegion->RectAt(0));
		CheckFrame(&tempRegion3);
		assert(!RegionIsEmpty(&tempRegion3));
		
		tempRegion1.Set(testRegion->RectAt(0));
		CheckFrame(&tempRegion1);
		assert(!RegionIsEmpty(&tempRegion1));
		
		assert(RegionsAreEqual(&tempRegion1, &tempRegion2));
		assert(RegionsAreEqual(&tempRegion1, &tempRegion3));
		assert(RegionsAreEqual(&tempRegion2, &tempRegion3));
	}
}


/*
 *  Method:  RegionConstruction::testTwoRegions()
 *   Descr:  This member function performs a test on two regions passed in.
 */	

void RegionConstruction::testTwoRegions(BRegion *testRegionA, BRegion *testRegionB)
{
	BRegion tempRegion1;
	CheckFrame(&tempRegion1);
	assert(RegionIsEmpty(&tempRegion1));
	
	tempRegion1 = *testRegionA;
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionA));
	
	tempRegion1 = *testRegionB;
	CheckFrame(&tempRegion1);
	assert(RegionsAreEqual(&tempRegion1, testRegionB));
	
	tempRegion1.MakeEmpty();
	CheckFrame(&tempRegion1);
	assert(RegionIsEmpty(&tempRegion1));
}
	

/*
 *  Method:  RegionConstruction::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "RegionConstruction".
 */

 Test *RegionConstruction::suite(void)
{	
	typedef CppUnit::TestCaller<RegionConstruction>
		RegionConstructionCaller;
		
	return(new RegionConstructionCaller("BRegion::Construction Test", &RegionConstruction::PerformTest));
	}
