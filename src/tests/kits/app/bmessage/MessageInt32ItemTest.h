//------------------------------------------------------------------------------
//	MessageInt32ItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEINT32ITEMTEST_H
#define MESSAGEINT32ITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
#define USE_TEMPLATES
#ifdef USE_TEMPLATES
#include "MessageItemTest.h"

typedef TMessageItemFuncPolicy
<
	int32,
	&BMessage::AddInt32,
	&BMessage::FindInt32,
	&BMessage::FindInt32,
	&BMessage::FindInt32,
	&BMessage::HasInt32,
	&BMessage::ReplaceInt32
>
TInt32FuncPolicy;

struct TInt32InitPolicy : public ArrayTypeBase<int32>
{
	inline static int32 Zero()	{ return 0; }
	inline static int32 Test1()	{ return 1234; }
	inline static int32 Test2()	{ return 5678; }
	inline static size_t SizeOf(const int32&)	{ return sizeof (int32); }
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
	int32
>
TInt32AssertPolicy;

typedef TMessageItemTest
<
	int32,
	B_INT32_TYPE,
	TInt32FuncPolicy,
	TInt32InitPolicy,
	TInt32AssertPolicy
>
TMessageInt32ItemTest;

#else
class TMessageInt32ItemTest : public TestCase
{
	public:
		TMessageInt32ItemTest() {;}
		TMessageInt32ItemTest(std::string name) : TestCase(name) {;}

		void MessageInt32ItemTest1();
		void MessageInt32ItemTest2();
		void MessageInt32ItemTest3();
		void MessageInt32ItemTest4();
		void MessageInt32ItemTest5();
		void MessageInt32ItemTest6();
		void MessageInt32ItemTest7();
		void MessageInt32ItemTest8();

		static TestSuite* Suite();
};
#endif

#endif	// MESSAGEINT32ITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

