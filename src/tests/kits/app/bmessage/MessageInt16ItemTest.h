//------------------------------------------------------------------------------
//	MessageInt16ItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEINT16ITEMTEST_H
#define MESSAGEINT16ITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	int16,
	&BMessage::AddInt16,
	&BMessage::FindInt16,
	&BMessage::FindInt16,
	&BMessage::FindInt16,
	&BMessage::HasInt16,
	&BMessage::ReplaceInt16
>
TInt16FuncPolicy;

struct TInt16InitPolicy : public ArrayTypeBase<int16>
{
	inline static int16 Zero()	{ return 0; }
	inline static int16 Test1()	{ return 1234; }
	inline static int16 Test2()	{ return 5678; }
	inline static size_t SizeOf(const int16&)	{ return sizeof (int16); }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(123);
		array.push_back(456);
		array.push_back(789);
		return array;
	}
};

typedef TMessageItemAssertPolicy
<
	int16
>
TInt16AssertPolicy;

typedef TMessageItemTest
<
	int16,
	B_INT16_TYPE,
	TInt16FuncPolicy,
	TInt16InitPolicy,
	TInt16AssertPolicy
>
TMessageInt16ItemTest;


#endif	// MESSAGEINT16ITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

