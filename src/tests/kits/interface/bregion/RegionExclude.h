/*
	$Id: RegionExclude.h 4235 2003-08-06 06:46:06Z jackburton $
	
	This file defines a class for performing one test of BRegion
	functionality.
	
	*/


#ifndef RegionExclude_H
#define RegionExclude_H


#include "RegionTestcase.h"

	
class RegionExclude :
	public RegionTestcase {
	
private:
	void CheckExclude(BRegion *, BRegion *, BRegion *);

protected:
	virtual void testOneRegion(BRegion *);
	virtual void testTwoRegions(BRegion *, BRegion *);

public:
	static Test *suite(void);
	RegionExclude(std::string name = "");
	virtual ~RegionExclude();
	};
	
#endif
