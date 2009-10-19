/*
	$Id: DummyPolygon.h 2067 2002-11-23 04:42:57Z jrand $
	
	This file contains the definition of a dummy BPolygon class.
	It is used in order to get access to the private points of the
	BPolygon.  The BPolygon class itself does not allow you to extract
	these points.
	
	This is a hack in order to properly test BPolygon.  It is highly
	dependent on the private structure of BPolygon and is likely to break
	if this structure is changed substantially in the future.  However,
	it is the only effective way to test BPolygon.
	
	*/ 

#ifndef DUMMYPOLYGON_H
#define DUMMYPOLYGON_H


#include <Rect.h>


class BPoint;


class DummyPolygon { 

public: 
	DummyPolygon(); 
	virtual ~DummyPolygon();   

	const BPoint *GetPoints(void);

/*----- Private or reserved -----------------------------------------*/ 
private: 
                BRect   fBounds; 
                int32   fCount; 
                BPoint  *fPts; 
}; 

#endif	// DUMMYPOLYGON_H






