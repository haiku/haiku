//------------------------------------------------------------------------------
//	MessageInt64ItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEINT64ITEMTEST_H
#define MESSAGEINT64ITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	int64,
	&BMessage::AddInt64,
	&BMessage::FindInt64,
	&BMessage::FindInt64,
	&BMessage::HasInt64,
	&BMessage::ReplaceInt64
>
TInt64FuncPolicy;

struct TInt64InitPolicy : public ArrayTypeBase<int64>
{
	inline static int64 Zero()	{ return 0; }
	inline static int64 Test1()	{ return 1234; }
	inline static int64 Test2()	{ return 5678; }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(123);
		array.push_back(456);
		array.push_back(789);
		return array;
	}
};

struct TInt64AssertPolicy
{
	inline static int64 Zero()		{ return 0; }
	inline static int64 Invalid()	{ return 0;}
};

typedef TMessageItemTest
<
	int64,
	B_INT64_TYPE,
	TInt64FuncPolicy,
	TInt64InitPolicy,
	TInt64AssertPolicy
>
TMessageInt64ItemTest;

#endif	// MESSAGEINT64ITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

