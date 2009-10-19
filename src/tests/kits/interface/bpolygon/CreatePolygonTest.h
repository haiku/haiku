/*
	$Id: CreatePolygonTest.h 2067 2002-11-23 04:42:57Z jrand $
	
	This file contains the definition of a class for testing the
	creation of BPolygon objects through a few different methods.
	
	*/ 


#ifndef CreatePolygonTest_H
#define CreatePolygonTest_H


#include "../common.h"


class BPolygon;
class BPoint;
class BRect;

	
class CreatePolygonTest :
	public TestCase {
	
private:
	void polygonMatches(BPolygon *, const BPoint *, int numPoints, BRect);
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	CreatePolygonTest(std::string name = "");
	virtual ~CreatePolygonTest();
	};
	
#endif




