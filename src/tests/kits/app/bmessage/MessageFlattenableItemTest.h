//------------------------------------------------------------------------------
//	MessageFlattenableItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEFLATTENABLEITEMTEST_H
#define MESSAGEFLATTENABLEITEMTEST_H

// Standard Includes -----------------------------------------------------------
#include <string>

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <Flattenable.h>

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------
#define MY_FLATTENABLE_TYPE	'flat'

// Globals ---------------------------------------------------------------------

class MyFlattenableType : public BFlattenable
{
	public:
		MyFlattenableType() {;}
		MyFlattenableType(const char* data)
			:	fData(data)
		{;}

		bool operator==(const MyFlattenableType& mft) const;
//			{ return fData == mft.fData; }

		virtual status_t	Flatten(void* buffer, ssize_t numBytes) const;
		virtual status_t	Unflatten(type_code code, const void* buffer,
									  ssize_t numBytes);
		virtual ssize_t		FlattenedSize() const { return fData.length() + 1; }
		virtual bool		IsFixedSize() const { return false; }
		virtual type_code	TypeCode() const { return MY_FLATTENABLE_TYPE; }

	private:
		std::string	fData;
};
//------------------------------------------------------------------------------
bool MyFlattenableType::operator==(const MyFlattenableType& mft) const
{
	bool ret = fData == mft.fData;
	return ret;
}
//------------------------------------------------------------------------------
status_t MyFlattenableType::Flatten(void* buffer, ssize_t numBytes) const
{
	if (!buffer)
	{
		return B_BAD_VALUE;
	}
	if (numBytes != FlattenedSize())
	{
		return B_NO_MEMORY;
	}
	memcpy(buffer, (const void*)fData.c_str(), fData.length());
	((char*)buffer)[fData.length()] = '\0';
	return B_OK;
}
//------------------------------------------------------------------------------
status_t MyFlattenableType::Unflatten(type_code code, const void* buffer,
									  ssize_t numBytes)
{
	if (!buffer)
	{
		return B_BAD_VALUE;
	}
	if (!AllowsTypeCode(code))
	{
		return B_ERROR;
	}
	fData.assign((const char*)buffer, numBytes - 1);
	return B_OK;
}
//------------------------------------------------------------------------------


struct TFlattenableFuncPolicy
{
	static status_t Add(BMessage& msg, const char* name, MyFlattenableType& val)
		{ return msg.AddFlat(name, &val); }
	static status_t AddData(BMessage& msg, const char* name, type_code type,
							MyFlattenableType* data, ssize_t size, bool)
	{
		char* buffer = new char[size];
		status_t err = data->Flatten(buffer, size);
		if (!err)
		{
			err = msg.AddData(name, type, buffer, size, false);
		}
		delete[] buffer;
		return err;
	}
	static status_t Find(BMessage& msg, const char* name, int32 index,
						 MyFlattenableType* val)
	{
		return msg.FindFlat(name, index, val);
	}
	static status_t ShortFind(BMessage& msg, const char* name, MyFlattenableType* val)
	{
		return msg.FindFlat(name, val);
	}
	static MyFlattenableType QuickFind(BMessage& msg, const char* name, int32 index)
	{
		MyFlattenableType mft;
		msg.FindFlat(name, index, &mft);
		return mft;
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
		{ return msg.HasFlat(name, index, &sFlat); }
	static status_t Replace(BMessage& msg, const char* name, int32 index,
							MyFlattenableType& val)
		{ return msg.ReplaceFlat(name, index, &val); }
	static status_t FindData(BMessage& msg, const char* name, type_code type,
							 int32 index, const void** data, ssize_t* size)
	{
		*data = NULL;
		char* ptr;
		status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
		if (!err)
		{
			err = sFlat.Unflatten(type, ptr, *size);
			if (!err)
			{
				*(MyFlattenableType**)data = &sFlat;
			}
		}
		return err;
	}

	private:
		static MyFlattenableType sFlat;
};
MyFlattenableType TFlattenableFuncPolicy::sFlat;
//------------------------------------------------------------------------------
struct TFlattenableInitPolicy : public ArrayTypeBase<MyFlattenableType>
{
	inline static MyFlattenableType Zero()	{ return ""; }
	inline static MyFlattenableType Test1()	{ return "flat1"; }
	inline static MyFlattenableType Test2()	{ return "MyFlattenableType"; }
	inline static size_t SizeOf(const BFlattenable& flat)
		{ return flat.FlattenedSize(); }
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
struct TFlattenableAssertPolicy
{
	inline static MyFlattenableType	Zero()				{ return MyFlattenableType(); }
	inline static MyFlattenableType	Invalid()			{ return MyFlattenableType(); }
	 static bool		Size(size_t size, MyFlattenableType& flat)
		;//{ return size == msg.FlattenedSize(); }
};
bool TFlattenableAssertPolicy::Size(size_t size, MyFlattenableType& flat)
{
	ssize_t flatSize = flat.FlattenedSize();
	return size == (size_t)flatSize;
}
//------------------------------------------------------------------------------
template<>
struct TypePolicy<MyFlattenableType>
{
	typedef MyFlattenableType* TypePtr;
	enum { FixedSize = false };
	inline MyFlattenableType& Dereference(TypePtr p)
	{
		return *p;
	}
	inline TypePtr AddressOf(MyFlattenableType& t) { return &t; }
};
//------------------------------------------------------------------------------

typedef TMessageItemTest
<
	MyFlattenableType,
	MY_FLATTENABLE_TYPE,
	TFlattenableFuncPolicy,
	TFlattenableInitPolicy,
	TFlattenableAssertPolicy
>
TMessageFlattenableItemTest;

#endif	// MESSAGEFLATTENABLEITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

