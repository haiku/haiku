//------------------------------------------------------------------------------
//	MessageDoubleItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEDOUBLEITEMTEST_H
#define MESSAGEDOUBLEITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	double,
	&BMessage::AddDouble,
	&BMessage::FindDouble,
	&BMessage::FindDouble,
	&BMessage::FindDouble,
	&BMessage::HasDouble,
	&BMessage::ReplaceDouble
>
TDoubleFuncPolicy;

struct TDoubleInitPolicy : public ArrayTypeBase<double>
{
	inline static double Zero()	{ return 0; }
	inline static double Test1()	{ return 1.234; }
	inline static double Test2()	{ return 5.678; }
	inline static size_t SizeOf(const double&)	{ return sizeof (double); }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(1.23);
		array.push_back(4.56);
		array.push_back(7.89);
		return array;
	}
};

struct TDoubleAssertPolicy
{
	inline static double Zero()		{ return 0; }
	inline static double Invalid()	{ return 0; }
	inline static bool   Size(size_t size, double& d)
		{ return size == sizeof (d); }
};

typedef TMessageItemTest
<
	double,
	B_DOUBLE_TYPE,
	TDoubleFuncPolicy,
	TDoubleInitPolicy,
	TDoubleAssertPolicy
>
TMessageDoubleItemTest;

#endif	// MESSAGEDOUBLEITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

