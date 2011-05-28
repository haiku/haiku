//------------------------------------------------------------------------------
//	MessageCStringItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGECSTRINGITEMTEST_H
#define MESSAGECSTRINGITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Debug.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	const char*,
	&BMessage::AddString,
	&BMessage::FindString,
	&BMessage::FindString,
	&BMessage::FindString,
	&BMessage::HasString,
	&BMessage::ReplaceString
>
TCStringFuncPolicy;

struct TCStringInitPolicy : public ArrayTypeBase<const char*>
{
	typedef const char*	TypePtr;

	inline static const char* Zero()	{ return sStr1; }
	inline static const char* Test1()	{ return sStr2; }
	inline static const char* Test2()	{ return sStr3; }
	inline static size_t SizeOf(const char*& data)	{ return strlen(data) + 1; }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}

	private:
		static const char* sStr1;
		static const char* sStr2;
		static const char* sStr3;
};
const char* TCStringInitPolicy::sStr1 = "";
const char* TCStringInitPolicy::sStr2 = "cstring one";
const char* TCStringInitPolicy::sStr3 = "Bibbity-bobbity-boo!";
//------------------------------------------------------------------------------
struct TCStringAssertPolicy
{
	inline static const char*	Zero()				{ return ""; }
	inline static const char*	Invalid()			{ return ""; }
	static bool					Size(size_t size, const char* data)
		;//{ return size == msg.FlattenedSize(); }
};
bool TCStringAssertPolicy::Size(size_t size, const char* data)
{
	return size == strlen(data) + 1;
}
//------------------------------------------------------------------------------
struct TCStringComparePolicy
{
	static bool Compare(const char* lhs, const char* rhs);
};
bool TCStringCtypedef const char* TypePtr;
	omparePolicy::Compare(const char* lhs, const char* rhs)
{
	if (!lhs)
		return rhs;
	if (!rhs)
		return lhs;
	return strcmp(lhs, rhs) == 0;
}
//------------------------------------------------------------------------------
template<>
struct TypePolicy<const char*>
{
	typedef const char* TypePtr;
	enum { FixedSize = false };
	inline const char* Dereference(TypePtr p) { return p; }
	inline const char* AddressOf(const char*& t) { return t; }
};
//------------------------------------------------------------------------------
typedef TMessageItemTest
<
	const char*,
	B_STRING_TYPE,
	TCStringFuncPolicy,
	TCStringInitPolicy,
	TCStringAssertPolicy,
	TCStringComparePolicy
>
TMessageCStringItemTest;


#endif	// MESSAGECSTRINGITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

