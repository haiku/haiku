/*
	$Id: RegionIntersect.h 4235 2003-08-06 06:46:06Z jackburton $
	
	This file defines a class for performing one test of BRegion
	functionality.
	
	*/


#ifndef RegionIntersect_H
#define RegionIntersect_H


#include "RegionTestcase.h"

	
class RegionIntersect :
	public RegionTestcase {
	
private:
	void CheckIntersect(BRegion *, BRegion *, BRegion *);

protected:
	virtual void testOneRegion(BRegion *);
	virtual void testTwoRegions(BRegion *, BRegion *);

public:
	static Test *suite(void);
	RegionIntersect(std::string name = "");
	virtual ~RegionIntersect();
	};
	
#endif
