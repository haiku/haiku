//------------------------------------------------------------------------------
//	MessageInt8ItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEINT8ITEMTEST_H
#define MESSAGEINT8ITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	int8,
	&BMessage::AddInt8,
	&BMessage::FindInt8,
	&BMessage::FindInt8,
	&BMessage::HasInt8,
	&BMessage::ReplaceInt8
>
TInt8FuncPolicy;

struct TInt8InitPolicy : public ArrayTypeBase<int8>
{
	inline static int8 Zero()	{ return 0; }
	inline static int8 Test1()	{ return 16; }
	inline static int8 Test2()	{ return 32; }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(64);
		array.push_back(128);
		array.push_back(255);
		return array;
	}
};

struct TInt8AssertPolicy
{
	inline static int8 Zero()		{ return 0; }
	inline static int8 Invalid()	{ return 0;}
};

typedef TMessageItemTest
<
	int8,
	B_INT8_TYPE,
	TInt8FuncPolicy,
	TInt8InitPolicy,
	TInt8AssertPolicy
>
TMessageInt8ItemTest;

#endif	// MESSAGEINT8ITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

