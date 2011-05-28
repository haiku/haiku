//------------------------------------------------------------------------------
//	MessagePointerItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEPOINTERITEMTEST_H
#define MESSAGEPOINTERITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

struct TPointerFuncPolicy
{
	static status_t Add(BMessage& msg, const char* name, const void* val)
		{ return msg.AddPointer(name, val); }
	static status_t AddData(BMessage& msg, const char* name, type_code type,
							const void* data, ssize_t size, bool)
		{ return msg.AddData(name, type, data, size); }
	static status_t Find(BMessage& msg, const char* name, int32 index,
						 const void** val)
		{ return msg.FindPointer(name, index, (void**)val); }
	static status_t ShortFind(BMessage& msg, const char* name, const void** val)
		{ return msg.FindPointer(name, (void**)val); }
	static const void* QuickFind(BMessage& msg, const char* name, int32 index)
	{
		const void* ptr;
		msg.FindPointer(name, index, (void**)&ptr);
		return ptr;
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
		{ return msg.HasPointer(name, index); }
	static status_t Replace(BMessage& msg, const char* name, int32 index,
							const void* val)
		{ return msg.ReplacePointer(name, index, val); }
	static status_t FindData(BMessage& msg, const char* name, type_code type,
							 int32 index, const void** data, ssize_t* size)
		{ return msg.FindData(name, type, index, data, size); }
};

struct TPointerInitPolicy : public ArrayTypeBase<const void*>
{
	inline static const void* Zero()	{ return NULL; }
	inline static const void* Test1()	{ return (const void*)&Test1; }
	inline static const void* Test2()	{ return (const void*)&Test2; }
	inline static size_t SizeOf(const void*)	{ return sizeof (const void*); }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}
};

struct TPointerAssertPolicy
{
	inline static const void* Zero()	{ return NULL; }
	inline static const void* Invalid()	{ return NULL; }
	inline static bool Size(size_t size, const void* t)
		{ return size == sizeof (t); }
};

typedef TMessageItemTest
<
	const void*,
	B_POINTER_TYPE,
	TPointerFuncPolicy,
	TPointerInitPolicy,
	TPointerAssertPolicy
>
TMessagePointerItemTest;

#endif	// MESSAGEPOINTERITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

