//------------------------------------------------------------------------------
//	MessageStringItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGESTRINGITEMTEST_H
#define MESSAGESTRINGITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Debug.h>
#include <String.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

struct TBStringFuncPolicy
{
	static status_t Add(BMessage& msg, const char* name, BString& data);
	static status_t Find(BMessage& msg, const char* name, int32 index,
						 BString* data);
	static status_t ShortFind(BMessage& msg, const char* name, BString* data);
	static BString QuickFind(BMessage& msg, const char* name, int32 index);
	static bool Has(BMessage& msg, const char* name, int32 index);
	static status_t Replace(BMessage& msg, const char* name, int32 index,
						 BString& data);
	static status_t AddData(BMessage& msg, const char* name, type_code type,
						 const BString* data, ssize_t size, bool);
	static status_t FindData(BMessage& msg, const char* name, type_code type,
						  int32 index, const void** data, ssize_t* size);

	private:
		static BString sStr;
};
BString TBStringFuncPolicy::sStr;
//------------------------------------------------------------------------------
status_t TBStringFuncPolicy::Add(BMessage& msg, const char* name,
									 BString& data)
{
	return msg.AddString(name, data);
}
//------------------------------------------------------------------------------
status_t TBStringFuncPolicy::Find(BMessage& msg, const char* name,
									  int32 index, BString* data)
{
	return msg.FindString(name, index, data);
}
//------------------------------------------------------------------------------
inline status_t TBStringFuncPolicy::ShortFind(BMessage& msg, const char* name,
											  BString* data)
{
	return msg.FindString(name, data);
}
//------------------------------------------------------------------------------
BString TBStringFuncPolicy::QuickFind(BMessage& msg, const char* name,
										  int32 index)
{
	BString data;
	msg.FindString(name, index, &data);
	return data;
}
//------------------------------------------------------------------------------
bool TBStringFuncPolicy::Has(BMessage& msg, const char* name,
								 int32 index)
{
	return msg.HasString(name, index);
}
//------------------------------------------------------------------------------
status_t TBStringFuncPolicy::Replace(BMessage& msg, const char* name,
										 int32 index, BString& data)
{
	return msg.ReplaceString(name, index, data);
}
//------------------------------------------------------------------------------
status_t TBStringFuncPolicy::AddData(BMessage& msg, const char* name,
										 type_code type, const BString* data,
										 ssize_t size, bool)
{
	return msg.AddData(name, type, (const void*)data->String(), size,
					   false);
}
//------------------------------------------------------------------------------
status_t TBStringFuncPolicy::FindData(BMessage& msg, const char* name,
										  type_code type, int32 index,
										  const void** data, ssize_t* size)
{
	*data = NULL;
	char* ptr;
	status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
	if (!err)
	{
		sStr = ptr;
		*(BString**)data = &sStr;
	}

	return err;
}
//------------------------------------------------------------------------------


struct TBStringInitPolicy : public ArrayTypeBase<BString>
{
	inline static BString Zero()	{ return BString(); }
	inline static BString Test1()	{ return "Test1"; }
	inline static BString Test2()	{ return "Supercalafragilistricexpialadocious"; }
	inline static size_t SizeOf(const BString& s)	{ return s.Length() + 1; }
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}
};
//------------------------------------------------------------------------------
struct TBStringAssertPolicy
{
	inline static BString	Zero()				{ return BString(); }
	inline static BString	Invalid()			{ return BString(); }
	 static bool			Size(size_t size, BString& msg)
		;//{ return size == msg.FlattenedSize(); }
};
bool TBStringAssertPolicy::Size(size_t size, BString& data)
{
	return size == (size_t)data.Length() + 1;
}
//------------------------------------------------------------------------------
typedef TMessageItemTest
<
	BString,
	B_STRING_TYPE,
	TBStringFuncPolicy,
	TBStringInitPolicy,
	TBStringAssertPolicy
>
TMessageBStringItemTest;

#endif	// MESSAGESTRINGITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

