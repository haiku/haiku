/*
	$Id: RegionOffsetBy.h,v 1.1 2003/08/06 06:46:06 jackburton Exp $
	
	This file defines a class for performing one test of BRegion
	functionality.
	
	*/


#ifndef RegionOffsetBy_H
#define RegionOffsetBy_H


#include "RegionTestcase.h"

	
class RegionOffsetBy :
	public RegionTestcase {
	
private:


protected:
	virtual void testOneRegion(BRegion *);
	virtual void testTwoRegions(BRegion *, BRegion *);

public:
	static Test *suite(void);
	RegionOffsetBy(std::string name = "");
	virtual ~RegionOffsetBy();
	};
	
#endif
