//------------------------------------------------------------------------------
//	MessageBPointItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEBPOINTITEMTEST_H
#define MESSAGEBPOINTITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	BPoint,
	&BMessage::AddPoint,
	&BMessage::FindPoint,
	&BMessage::FindPoint,
	&BMessage::FindPoint,
	&BMessage::HasPoint,
	&BMessage::ReplacePoint
>
TPointFuncPolicy;

struct TPointInitPolicy : public ArrayTypeBase<BPoint>
{
	inline static BPoint Zero()		{ return BPoint(0.0, 0.0); }
	inline static BPoint Test1()	{ return BPoint(10.0, 10.0); }
	inline static BPoint Test2()	{ return BPoint(20.0, 20.0); }
	inline static size_t SizeOf(const BPoint&)	{ return sizeof (BPoint); }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(BPoint(30.0, 30.0));
		array.push_back(BPoint(40.0, 40.0));
		array.push_back(BPoint(50.0, 50.0));
		return array;
	}
};

struct TPointAssertPolicy
{
	inline static BPoint Zero()		{ return BPoint(0.0, 0.0); }
	inline static BPoint Invalid()	{ return BPoint(0.0, 0.0); }
	inline static bool   Size(size_t size, BPoint& p)
		{ return size == sizeof (p); }	
};

typedef TMessageItemTest
<
	BPoint,
	B_POINT_TYPE,
	TPointFuncPolicy,
	TPointInitPolicy,
	TPointAssertPolicy
>
TMessageBPointItemTest;

std::ostream& operator<<(std::ostream& os, const BPoint& point)
{
	int precision = os.precision();
	os << "point(x:" << point.x << ", y:" << point.y;
	os.precision(precision);
	return os;
}

#endif	// MESSAGEBPOINTITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

