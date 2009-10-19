/*
	$Id: MapPolygonTest.h 2136 2002-12-03 04:07:21Z jrand $
	
	This file contains the definition of a class for testing the
	mapping of BPolygon objects.
	
	*/ 


#ifndef MapPolygonTest_H
#define MapPolygonTest_H


#include "../common.h"


class BPolygon;
class BPoint;
class BRect;

	
class MapPolygonTest :
	public TestCase {
	
private:
	void polygonMatches(BPolygon *, const BPoint *, int numPoints, BRect);
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	MapPolygonTest(std::string name = "");
	virtual ~MapPolygonTest();
	};
	
#endif




