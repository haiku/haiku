/*
	$Id: DummyPolygon.cpp,v 1.1 2002/11/23 04:42:57 jrand Exp $
	
	This file contains the implementation of the dummy BPolygon
	class which allows the tests to grab the set of points from
	BPolygon's private members.  This is the only way to effectively
	test BPolygon.
	
	*/


#include "DummyPolygon.h"


DummyPolygon::DummyPolygon()
{
	throw "You cannot create a DummyPolygon, just cast existing BPolygon's to it!";
	}
	
	
DummyPolygon::~DummyPolygon()
{
	}
	
	
const BPoint *DummyPolygon::GetPoints(void)
{
	return(fPts);
	}