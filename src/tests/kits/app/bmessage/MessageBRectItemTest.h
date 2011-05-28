//------------------------------------------------------------------------------
//	MessageBRectItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEBRECTITEMTEST_H
#define MESSAGEBRECTITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	BRect,
	&BMessage::AddRect,
	&BMessage::FindRect,
	&BMessage::FindRect,
	&BMessage::FindRect,
	&BMessage::HasRect,
	&BMessage::ReplaceRect
>
TBRectFuncPolicy;

struct TBRectInitPolicy : public ArrayTypeBase<BRect>
{
	inline static BRect Zero()		{ return BRect(0, 0, 0, 0); }
	inline static BRect Test1()	{ return BRect(1, 2, 3, 4); }
	inline static BRect Test2()	{ return BRect(5, 6, 7, 8); }
	inline static size_t SizeOf(const BRect&)	{ return sizeof (BRect); }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(BRect(1, 2, 3, 4));
		array.push_back(BRect(4, 5, 6, 7));
		array.push_back(BRect(7, 8, 9, 10));
		return array;
	}
};

struct TBRectAssertPolicy
{
	inline static BRect Zero() { return BRect(0, 0, 0, 0); }
	inline static BRect Invalid() { return BRect(0, 0, -1, -1); }
	inline static bool  Size(size_t size, BRect& r)
		{ return size == sizeof (r); }
};

typedef TMessageItemTest
<
	BRect,
	B_RECT_TYPE,
	TBRectFuncPolicy,
	TBRectInitPolicy,
	TBRectAssertPolicy
>
TMessageBRectItemTest;

std::ostream& operator<<(std::ostream& os, const BRect& rect)
{
	int precision = os.precision();
	os.precision(1);
	os << "rect"
	   << "(l:" << rect.left
	   << " t:" << rect.top
	   << " r:" << rect.right
	   << " b:" << rect.bottom
	   << ")";
	os.precision(precision);

	return os;
}

#endif	// MESSAGEBRECTITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

