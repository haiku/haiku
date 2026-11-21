/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct PointerFuncPolicy : public TypePolicy<const void*> {
	static status_t Add(BMessage& msg, const char* name, const void* val)
	{
		return msg.AddPointer(name, val);
	}
	static status_t AddData(BMessage& msg, const char* name, type_code type, const void* data,
							ssize_t size, bool)
	{
		return msg.AddData(name, type, data, size);
	}
	static status_t Find(BMessage& msg, const char* name, int32 index, const void** val)
	{
		return msg.FindPointer(name, index, (void**)val);
	}
	static status_t ShortFind(BMessage& msg, const char* name, const void** val)
	{
		return msg.FindPointer(name, (void**)val);
	}
	static const void* QuickFind(BMessage& msg, const char* name, int32 index)
	{
		const void* ptr;
		msg.FindPointer(name, index, (void**)&ptr);
		return ptr;
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasPointer(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index, const void* val)
	{
		return msg.ReplacePointer(name, index, val);
	}
	static status_t FindData(BMessage& msg, const char* name, type_code type, int32 index,
							 const void** data, ssize_t* size)
	{
		return msg.FindData(name, type, index, data, size);
	}
};


struct PointerInitPolicy : public ArrayTypeBase<const void*> {
	static const void* Zero() { return NULL; }
	static const void* Test1() { return (const void*)&Test1; }
	static const void* Test2() { return (const void*)&Test2; }
	static size_t SizeOf(const void*) { return sizeof(const void*); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}
};


struct PointerAssertPolicy {
	static const void* Zero() { return NULL; }
	static const void* Invalid() { return NULL; }
	static bool Size(size_t size, const void* t) { return size == sizeof(t); }
};

typedef
MessageItemTest<const void*, B_POINTER_TYPE, PointerFuncPolicy,
		PointerInitPolicy, PointerAssertPolicy>
	MessagePointerItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessagePointerItemTest, getTestSuiteName());
