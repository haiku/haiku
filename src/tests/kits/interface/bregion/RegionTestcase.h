/*
	$Id: RegionTestcase.h 4235 2003-08-06 06:46:06Z jackburton $
	
	This file defines a base class for performing all tests of BRegion
	functionality.
	
	*/


#ifndef RegionTestcase_H
#define RegionTestcase_H


#include "../common.h"
#include <List.h>
#include <Rect.h>
#include <Point.h>


class BRegion;

	
class RegionTestcase :
	public TestCase {
	
private:
	BList listOfRegions;
	
#define numPointsPerSide 17
	BPoint pointArray[numPointsPerSide * numPointsPerSide];
	
protected:
	int GetPointsInRect(BRect, BPoint **);
	void CheckFrame(BRegion *);
	bool RegionsAreEqual(BRegion *, BRegion *);
	bool RegionIsEmpty(BRegion *);

	virtual void testOneRegion(BRegion *) = 0;
	virtual void testTwoRegions(BRegion *, BRegion *) = 0;
	
public:
	void PerformTest(void);
	RegionTestcase(std::string name = "");
	virtual ~RegionTestcase();
	};
	
#endif
