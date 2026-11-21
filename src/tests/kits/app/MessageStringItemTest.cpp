/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct BStringFuncPolicy : public TypePolicy<BString> {
	static status_t FindData(BMessage& msg, const char* name, type_code type, int32 index,
							 const void** data, ssize_t* size)
	{
		*data = NULL;
		char* ptr;
		status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
		if (!err) {
			sStr = ptr;
			*(BString**)data = &sStr;
		}

		return err;
	}

	static status_t AddData(BMessage& msg, const char* name, type_code type, const BString* data,
							ssize_t size, bool)
	{
		return msg.AddData(name, type, (const void*)data->String(), size, false);
	}

	static status_t Replace(BMessage& msg, const char* name, int32 index, BString& data)
	{
		return msg.ReplaceString(name, index, data);
	}

	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasString(name, index);
	}

	static BString QuickFind(BMessage& msg, const char* name, int32 index)
	{
		BString data;
		msg.FindString(name, index, &data);
		return data;
	}

	static status_t ShortFind(BMessage& msg, const char* name, BString* data)
	{
		return msg.FindString(name, data);
	}

	static status_t Find(BMessage& msg, const char* name, int32 index, BString* data)
	{
		return msg.FindString(name, index, data);
	}

	static status_t Add(BMessage& msg, const char* name, BString& data)
	{
		return msg.AddString(name, data);
	}

private:
	static BString sStr;
};


BString BStringFuncPolicy::sStr;


struct BStringInitPolicy : public ArrayTypeBase<BString> {
	static BString Zero() { return BString(); }
	static BString Test1() { return "Test1"; }
	static BString Test2() { return "Supercalafragilistricexpialadocious"; }
	static size_t SizeOf(const BString& s) { return s.Length() + 1; }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}
};


struct BStringAssertPolicy {
	static BString Zero() { return BString(); }
	static BString Invalid() { return BString(); }

	static bool Size(size_t size, BString& data) { return size == (size_t)data.Length() + 1; }
};


template<> struct TypePolicy<const char*> {
	typedef const char** TypePtr;
	enum { FixedSize = false };
	static const char* Dereference(TypePtr p) { return *p; }
	static TypePtr AddressOf(const char*& t) { return &t; }
};


struct CStringInitPolicy : public ArrayTypeBase<const char*> {
	typedef const char* TypePtr;

	static const char* Zero() { return sStr1; }
	static const char* Test1() { return sStr2; }
	static const char* Test2() { return sStr3; }
	static size_t SizeOf(const char*& data) { return strlen(data) + 1; }
	static ArrayType Array()
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


const char* CStringInitPolicy::sStr1 = "";
const char* CStringInitPolicy::sStr2 = "cstring one";
const char* CStringInitPolicy::sStr3 = "Bibbity-bobbity-boo!";


struct CStringAssertPolicy {
	static const char* Zero() { return ""; }
	static const char* Invalid() { return ""; }

	static bool Size(size_t size, const char* data) { return size == strlen(data) + 1; }
};


struct CStringComparePolicy {
	static bool Compare(const char* lhs, const char* rhs)
	{
		if (!lhs)
			return rhs;
		if (!rhs)
			return lhs;
		return strcmp(lhs, rhs) == 0;
	}
	typedef const char* TypePtr;
};

typedef
MessageItemFuncPolicy<const char*, &BMessage::AddString, &BMessage::FindString,
		&BMessage::FindString, &BMessage::FindString, &BMessage::HasString,
		&BMessage::ReplaceString>
	CStringFuncPolicy;


typedef
MessageItemTest<BString, B_STRING_TYPE, BStringFuncPolicy, BStringInitPolicy,
		BStringAssertPolicy>
	MessageBStringItemTest;
typedef
MessageItemTest<const char*, B_STRING_TYPE, CStringFuncPolicy, CStringInitPolicy,
		CStringAssertPolicy, CStringComparePolicy>
	MessageCStringItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageBStringItemTest, getTestSuiteName());
// CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageCStringItemTest, getTestSuiteName());
	// FIXME: crash
