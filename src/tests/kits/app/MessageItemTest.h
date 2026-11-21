/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _APP_MESSAGE_ITEMTEST_H_
#define _APP_MESSAGE_ITEMTEST_H_


// A sad attempt to get rid of the horrible and pathetic vector<bool>
// specialization
#define __SGI_STL_INTERNAL_BVECTOR_H
#include <vector>

#include <stdio.h>
#include <string.h>

#include <Message.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


template<class T> struct ArrayTypeBase {
	typedef std::vector<T> ArrayType;
	typedef typename ArrayType::size_type SizeType;
	static SizeType Size(ArrayType& array) { return array.size(); }
};


template<class Type> struct TypePolicy {
	enum { FixedSize = true };
	static Type& Dereference(Type* p)
	{
		return *p;
	}
	static Type* AddressOf(Type& t) { return &t; }
};


template<typename Type, status_t (BMessage::*AddFunc)(const char*, Type),
		 status_t (BMessage::*FindFunc)(const char*, int32, Type*) const,
		 status_t (BMessage::*ShortFindFunc)(const char*, Type*) const,
		 Type (BMessage::*QuickFindFunc)(const char*, int32) const,
		 bool (BMessage::*HasFunc)(const char*, int32) const,
		 status_t (BMessage::*ReplaceFunc)(const char*, int32, Type),
		 status_t (BMessage::*AddDataFunc)(const char*, type_code, const void*,
										   ssize_t, bool, int32)
		 = &BMessage::AddData,
		 status_t (BMessage::*FindDataFunc)(const char*, type_code, int32,
											const void**, ssize_t*) const
		 = &BMessage::FindData>
struct MessageItemFuncPolicy : public TypePolicy<Type> {
	static status_t Add(BMessage& msg, const char* name, Type& val)
	{
		return (msg.*AddFunc)(name, val);
	}
	static status_t AddData(BMessage& msg, const char* name, type_code type,
		Type* val, ssize_t size, bool fixedSize = true)
	{
		return (msg.*AddDataFunc)(name, type, (const void*)val, size, fixedSize, 1);
	}
	static status_t Find(BMessage& msg, const char* name, int32 index, Type* val)
	{
		return (msg.*FindFunc)(name, index, val);
	}
	static status_t ShortFind(BMessage& msg, const char* name, Type* val)
	{
		return (msg.*ShortFindFunc)(name, val);
	}
	static Type QuickFind(BMessage& msg, const char* name, int32 index)
	{
		return (msg.*QuickFindFunc)(name, index);
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return (msg.*HasFunc)(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index, Type& val)
	{
		return (msg.*ReplaceFunc)(name, index, val);
	}
	static status_t FindData(BMessage& msg, const char* name, type_code type,
							 int32 index, const void** data, ssize_t* size)
	{
		return (msg.*FindDataFunc)(name, type, index, data, size);
	}
};


template<class T, T zero = T(), T invalid = T()>
struct MessageItemAssertPolicy {
	static T Zero()		{ return zero; }
	static T Invalid()	{ return invalid; }
	static bool Size(size_t size, T& t)
		{ return size == sizeof (t); }
};


template<class T> struct MessageItemComparePolicy {
	static bool
	Compare(const T &lhs, const T &rhs)
	{
		 return lhs == rhs;
	}
};


/**
	\tparam Type			native type (e.g. int32)
	\tparam TypeCode		BMessage type code (e.g. B_INT32_TYPE)
	\tparam FuncPolicy		see MessageItemFuncPolicy template
	\tparam InitPolicy		interface based on ArrayTypeBase and initialization
							methods: Type Zero(), Type Test1(), Type Test2(),
							size_t Sizeof(const Type&)
	\tparam	AssertPolicy	see MessageItemAssertPolicy
	\tparam ComparePolicy	see MessageItemComparePolicy
*/
template<class Type, type_code TypeCode, class FuncPolicy, class InitPolicy,
		 class AssertPolicy, class ComparePolicy = MessageItemComparePolicy<Type> >
class MessageItemTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MessageItemTest);
	CPPUNIT_TEST(EmptyMessage_FindAtIndexZero_ReturnsNameNotFound);
	CPPUNIT_TEST(SingleItem_AddAndFind_ReturnsExpected);
	CPPUNIT_TEST(SingleItem_Replace_ReturnsExpected);
	CPPUNIT_TEST(EmptyMessage_FindAtIndexOne_ReturnsNameNotFound);
	CPPUNIT_TEST(MultipleItems_AddAndFind_ReturnsExpected);
	CPPUNIT_TEST(MultipleItems_ReplaceAtIndex_ReturnsExpected);
	CPPUNIT_TEST(SingleItem_AddDataAndFind_ReturnsExpected);
	CPPUNIT_TEST(MultipleItems_AddDataAndFind_ReturnsExpected);
#ifndef TEST_R5
	CPPUNIT_TEST(NullName_Add_ReturnsBadValue);
	CPPUNIT_TEST(NullName_Find_ReturnsBadValue);
#endif
	CPPUNIT_TEST(SingleItem_FlattenAndUnflatten_ReturnsExpected);
	CPPUNIT_TEST(MultipleItems_FlattenAndUnflatten_ReturnsExpected);
	CPPUNIT_TEST_SUITE_END();

public:
	void EmptyMessage_FindAtIndexZero_ReturnsNameNotFound()
	{
		BMessage msg;
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT_EQUAL(B_NAME_NOT_FOUND, FuncPolicy::ShortFind(msg, "item", &out));
		CPPUNIT_ASSERT_EQUAL(B_NAME_NOT_FOUND, FuncPolicy::Find(msg, "item", 0, &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, AssertPolicy::Invalid()));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0),
												AssertPolicy::Invalid()));
		CPPUNIT_ASSERT(!FuncPolicy::Has(msg, "item", 0));
		const void* ptr = &out;
		ssize_t size;
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, 0, &ptr, &size) ==
						B_NAME_NOT_FOUND);
		CPPUNIT_ASSERT(ptr == NULL);
	}

	void SingleItem_AddAndFind_ReturnsExpected()
	{
		BMessage msg;
		Type in = InitPolicy::Test1();
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in));
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0), in));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::ShortFind(msg, "item", &out));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", 0, &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in));
		Type* pout = NULL;
		ssize_t size;
		status_t err = FuncPolicy::FindData(msg, "item", TypeCode, 0,
											(const void**)&pout, &size);
		CPPUNIT_ASSERT_EQUAL(B_OK, err);
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
	}

	void SingleItem_Replace_ReturnsExpected()
	{
		BMessage msg;
		Type in = InitPolicy::Test1();
		Type in2 = InitPolicy::Test2();
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Replace(msg, "item", 0, in2));
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0), in2));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::ShortFind(msg, "item", &out));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", 0, &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in2));
		out = InitPolicy::Zero();
		Type* pout;
		ssize_t size;
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, 0,
											(const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in2));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
	}

	void EmptyMessage_FindAtIndexOne_ReturnsNameNotFound()
	{
		BMessage msg;
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT_EQUAL(B_NAME_NOT_FOUND, FuncPolicy::Find(msg, "item", 1, &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, AssertPolicy::Invalid()));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 1),
												AssertPolicy::Invalid()));
		CPPUNIT_ASSERT(!FuncPolicy::Has(msg, "item", 1));
		const void* ptr = &out;
		ssize_t size;
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, 1, &ptr, &size) ==
						B_NAME_NOT_FOUND);
		CPPUNIT_ASSERT(ptr == NULL);
	}

	void MultipleItems_AddAndFind_ReturnsExpected()
	{
		BMessage msg;
		ArrayType in = InitPolicy::Array();
		Type out = InitPolicy::Zero();
		Type* pout;
		ssize_t size;

		for (uint32 ii = 0; ii < InitPolicy::Size(in); ++ii)
		{
			CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in[ii]));
		}

		for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
		{
			CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
			CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", i),
													in[i]));
			CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", i, &out));
			CPPUNIT_ASSERT(ComparePolicy::Compare(out, in[i]));
			CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, i,
												(const void**)&pout, &size) == B_OK);
			CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in[i]));
			CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
		}
	}

	void MultipleItems_ReplaceAtIndex_ReturnsExpected()
	{
		BMessage msg;
		ArrayType in = InitPolicy::Array();
		Type in2 = InitPolicy::Test2();
		Type out = InitPolicy::Zero();
		const int rIndex = 2;

		for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
		{
			CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in[i]));
		}

		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Replace(msg, "item", rIndex, in2));
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", rIndex));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", rIndex),
												in2));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", rIndex, &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in2));
		out = InitPolicy::Zero();
		Type* pout;
		ssize_t size;
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, rIndex,
											(const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in2));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
	}

	void SingleItem_AddDataAndFind_ReturnsExpected()
	{
		BMessage msg;
		Type in = InitPolicy::Test1();
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT(FuncPolicy::AddData(msg, "item", TypeCode, FuncPolicy::AddressOf(in),
											InitPolicy::SizeOf(in),
											TypePolicy<Type>::FixedSize) == B_OK);
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0),
												in));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::ShortFind(msg, "item", &out));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", 0, &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in));
		Type* pout = NULL;
		ssize_t size;
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, 0,
											(const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
	}

	void MultipleItems_AddDataAndFind_ReturnsExpected()
	{
		BMessage msg;
		ArrayType in = InitPolicy::Array();
		Type out = InitPolicy::Zero();
		Type* pout;
		ssize_t size;
		for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
		{
			CPPUNIT_ASSERT(FuncPolicy::AddData(msg, "item", TypeCode,
							FuncPolicy::AddressOf(in[i]), InitPolicy::SizeOf(in[i]),
							TypePolicy<Type>::FixedSize) == B_OK);
		}

		for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
		{
			CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
			CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", i),
													in[i]));
			CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", i, &out));
			CPPUNIT_ASSERT(ComparePolicy::Compare(out, in[i]));
			CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, i,
												(const void**)&pout, &size) == B_OK);
			CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in[i]));
			CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
		}
	}

	void NullName_Add_ReturnsBadValue()
	{
		BMessage msg;
		Type in = InitPolicy::Test1();
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, FuncPolicy::Add(msg, NULL, in));
	}

	void NullName_Find_ReturnsBadValue()
	{
		BMessage msg;
		Type in = InitPolicy::Test1();
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in));
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, NULL, 0) == false);
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, NULL, 0),
												AssertPolicy::Invalid()));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, FuncPolicy::ShortFind(msg, NULL, &out));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, FuncPolicy::Find(msg, NULL, 0, &out));
		Type* pout = NULL;
		ssize_t size;
		status_t err = FuncPolicy::FindData(msg, NULL, TypeCode, 0,
											(const void**)&pout, &size);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, err);
	}

	void SingleItem_FlattenAndUnflatten_ReturnsExpected()
	{
		BMessage msg;
		Type in = InitPolicy::Test1();
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in));

		ssize_t flatSize = msg.FlattenedSize();
		char* buf = new char[flatSize];
		CPPUNIT_ASSERT(buf);

		CPPUNIT_ASSERT_EQUAL(B_OK, msg.Flatten(buf, flatSize));

		BMessage msg2;
		Type out = InitPolicy::Zero();
		CPPUNIT_ASSERT_EQUAL(B_OK, msg2.Unflatten(buf));
		CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::ShortFind(msg, "item", &out));
		CPPUNIT_ASSERT(ComparePolicy::Compare(in, out));

		delete[] buf;
	}

	void MultipleItems_FlattenAndUnflatten_ReturnsExpected()
	{
		BMessage msg;
		ArrayType in = InitPolicy::Array();

		for (uint32 ii = 0; ii < InitPolicy::Size(in); ++ii)
		{
			CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Add(msg, "item", in[ii]));
		}

		ssize_t flatSize = msg.FlattenedSize();
		char* buf = new char[flatSize];
		CPPUNIT_ASSERT(buf);

		CPPUNIT_ASSERT_EQUAL(B_OK, msg.Flatten(buf, flatSize));

		BMessage msg2;
		Type out = InitPolicy::Zero();
		Type* pout;
		ssize_t size;
		CPPUNIT_ASSERT_EQUAL(B_OK, msg2.Unflatten(buf));

		for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
		{
			CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
			CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", i),
													in[i]));
			CPPUNIT_ASSERT_EQUAL(B_OK, FuncPolicy::Find(msg, "item", i, &out));
			CPPUNIT_ASSERT(ComparePolicy::Compare(out, in[i]));
			CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, i,
												(const void**)&pout, &size) == B_OK);
			CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::Dereference(pout), in[i]));
			CPPUNIT_ASSERT(AssertPolicy::Size(size, FuncPolicy::Dereference(pout)));
		}

		delete[] buf;
	}

	typedef typename InitPolicy::ArrayType ArrayType;
};


#endif // _APP_MESSAGE_ITEMTEST_H_
