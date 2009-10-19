/*
	$Id: RegionInclude.h 4235 2003-08-06 06:46:06Z jackburton $
	
	This file defines a class for performing one test of BRegion
	functionality.
	
	*/


#ifndef RegionInclude_H
#define RegionInclude_H


#include "RegionTestcase.h"

	
class RegionInclude :
	public RegionTestcase {
	
private:
	void CheckInclude(BRegion *, BRegion *, BRegion *);

protected:
	virtual void testOneRegion(BRegion *);
	virtual void testTwoRegions(BRegion *, BRegion *);

public:
	static Test *suite(void);
	RegionInclude(std::string name = "");
	virtual ~RegionInclude();
	};
	
#endif
