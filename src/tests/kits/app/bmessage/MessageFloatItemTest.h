//------------------------------------------------------------------------------
//	MessageFloatItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEFLOATITEMTEST_H
#define MESSAGEFLOATITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	float,
	&BMessage::AddFloat,
	&BMessage::FindFloat,
	&BMessage::FindFloat,
	&BMessage::FindFloat,
	&BMessage::HasFloat,
	&BMessage::ReplaceFloat
>
TFloatFuncPolicy;

struct TFloatInitPolicy : public ArrayTypeBase<float>
{
	inline static float Zero()	{ return 0; }
	inline static float Test1()	{ return 1.234; }
	inline static float Test2()	{ return 5.678; }
	inline static size_t SizeOf(const float&)	{ return sizeof (float); }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(1.23);
		array.push_back(4.56);
		array.push_back(7.89);
		return array;
	}
};

struct TFloatAssertPolicy
{
	inline static float Zero()		{ return 0; }
	inline static float Invalid()	{ return 0; }
	inline static bool  Size(size_t size, float& f)
		{ return size == sizeof (f); }
};

typedef TMessageItemTest
<
	float,
	B_FLOAT_TYPE,
	TFloatFuncPolicy,
	TFloatInitPolicy,
	TFloatAssertPolicy
>
TMessageFloatItemTest;


#endif	// MESSAGEFLOATITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

