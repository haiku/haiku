//------------------------------------------------------------------------------
//	MessageItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEITEMTEST_H
#define MESSAGEITEMTEST_H

// A sad attempt to get rid of the horrible and pathetic vector<bool> specialization
#define __SGI_STL_INTERNAL_BVECTOR_H

// Standard Includes -----------------------------------------------------------
#include <iostream>
#include <stdio.h>
#ifdef TEST_OBOS
#include <posix/typeinfo>
#else
#include <typeinfo>
#endif
#include <posix/string.h>

// System Includes -------------------------------------------------------------
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------
#define TEMPLATE_TEST_PARAMS <Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>
#define ADD_TEMPLATE_TEST(classbeingtested, suitename, classname, funcname)				\
	(suitename)->addTest(new TestCaller<classname>(((string(#classbeingtested) + "::" + #funcname + "::" + typeid(Type).name()).c_str() ),	\
						 &classname::funcname));

// Globals ---------------------------------------------------------------------
template
<
	class Type,				// int32
	type_code TypeCode,		// B_INT32_TYPE
	class FuncPolicy,		// status_t Add(BMessage&, const char*, Type&)
							// status_t Find(BMessage&, const char*, int32, Type*)
							// Type QuickFind(BMessage&, const char*, int32)
							// bool Has(BMessage&, const char*, int32)
							// status_t Replace(BMessage&, const char*, int32, Type)
	class InitPolicy,		// Type Zero()
							// Type Test1()
							// Type Test2()
							// vector<Type> Array()
	class AssertPolicy		// Type Zero()
>
class TMessageItemTest;


//------------------------------------------------------------------------------
template<class T>
struct ArrayTypeBase
{
	typedef vector<T> ArrayType;
	typedef typename ArrayType::size_type SizeType;
	static SizeType Size(ArrayType& array) { return array.size(); }
};
//------------------------------------------------------------------------------
template
<
	typename Type,
	status_t (BMessage::*AddFunc)(const char*, Type),
	status_t (BMessage::*FindFunc)(const char*, int32, Type*) const,
	Type (BMessage::*QuickFindFunc)(const char*, int32) const,
	bool (BMessage::*HasFunc)(const char*, int32) const,
	status_t (BMessage::*ReplaceFunc)(const char*, int32, Type)
>
struct TMessageItemFuncPolicy
{
	static status_t Add(BMessage& msg, const char* name, Type& val)
	{
		return (msg.*AddFunc)(name, val);
	}
	static status_t Find(BMessage& msg, const char* name, int32 index, Type* val)
	{
		return (msg.*FindFunc)(name, index, val);
	}
	static Type QuickFind(BMessage& msg, const char* name, int32 index)
	{
		return (msg.*QuickFindFunc)(name, index);
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return (msg.*HasFunc)(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index, Type val)
	{
		return (msg.*ReplaceFunc)(name, index, val);
	}
};
//------------------------------------------------------------------------------
template
<
	class Type,				// int32
	type_code TypeCode,		// B_INT32_TYPE
	class FuncPolicy,		// status_t Add(BMessage&, const char*, Type&)
							// status_t Find(BMessage&, const char*, int32, Type*)
							// Type QuickFind(BMessage&, const char*, int32)
							// bool Has(BMessage&, const char*, int32)
							// status_t Replace(BMessage&, const char*, int32, Type)
	class InitPolicy,		// Type Zero()
							// Type Test1()
							// Type Test2()
							// ArrayType Array()
							// typedef XXX ArrayType
	class AssertPolicy		// Type Zero()
							// Type Invalid()
>
class TMessageItemTest : public TestCase
{
	public:
		TMessageItemTest() {;}
		TMessageItemTest(std::string name) : TestCase(name) {;}

		void MessageItemTest1();
		void MessageItemTest2();
		void MessageItemTest3();
		void MessageItemTest4();
		void MessageItemTest5();
		void MessageItemTest6();
		void MessageItemTest7();
		void MessageItemTest8();

		static TestSuite* Suite();

	typedef typename InitPolicy::ArrayType ArrayType;
};


//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest1()
{
	BMessage msg;
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(out == AssertPolicy::Invalid());
	CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", 0) == AssertPolicy::Invalid());
	CPPUNIT_ASSERT(!FuncPolicy::Has(msg, "item", 0));
	const void* ptr = &out;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("item", TypeCode, &ptr, &size) ==
				   B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(ptr == NULL);
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest2()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
	CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", 0) == in);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_OK);
	CPPUNIT_ASSERT(out == in);
	Type* pout = NULL;
	ssize_t size;
	status_t err = msg.FindData("item", TypeCode, (const void**)&pout, &size);
	CPPUNIT_ASSERT(err == B_OK);
	CPPUNIT_ASSERT(*pout == in);
	CPPUNIT_ASSERT(size == sizeof (Type));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest3()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type in2 = InitPolicy::Test2();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Replace(msg, "item", 0, in2) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
	CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", 0) == in2);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_OK);
	CPPUNIT_ASSERT(out == in2);
	out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("item", TypeCode, (const void**)&pout,
				   &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in2);
	CPPUNIT_ASSERT(size == sizeof (Type));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest4()
{
	BMessage msg;
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 1, &out) == B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(out == AssertPolicy::Invalid());
	CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", 1) == AssertPolicy::Invalid());
	CPPUNIT_ASSERT(!FuncPolicy::Has(msg, "item", 1));
	const void* ptr = &out;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("item", TypeCode, 1, &ptr, &size) ==
				   B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(ptr == NULL);
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest5()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	Type out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	
	for (int32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in[i]) == B_OK);
	}

	for (int32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
		CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", i) == in[i]);
		CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", i, &out) == B_OK);
		CPPUNIT_ASSERT(out == in[i]);
		CPPUNIT_ASSERT(msg.FindData("item", TypeCode, i,
					   (const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(*pout == in[i]);
		CPPUNIT_ASSERT(size == sizeof (Type));
	}
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest6()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	Type in2 = InitPolicy::Test2();
	Type out = InitPolicy::Zero();
	const int rIndex = 2;

	for (int32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in[i]) == B_OK);
	}

	CPPUNIT_ASSERT(FuncPolicy::Replace(msg, "item", rIndex, in2) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", rIndex));
	CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", rIndex) == in2);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", rIndex, &out) == B_OK);
	CPPUNIT_ASSERT(out == in2);
	out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("item", TypeCode, rIndex,
				   (const void**)&pout, &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in2);
	CPPUNIT_ASSERT(size == sizeof (Type));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest7()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(msg.AddData("item", TypeCode, (const void*)&in,
							   sizeof (Type)) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
	CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", 0) == in);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_OK);
	CPPUNIT_ASSERT(out == in);
	Type* pout = NULL;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("item", TypeCode, (const void**)&pout,
				   &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in);
	CPPUNIT_ASSERT(size == sizeof (Type));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
MessageItemTest8()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	Type out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	
	for (int32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(msg.AddData("item", TypeCode,
					   (const void*)&in[i], sizeof (Type)) == B_OK);
	}

	for (int32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
		CPPUNIT_ASSERT(FuncPolicy::QuickFind(msg, "item", i) == in[i]);
		CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", i, &out) == B_OK);
		CPPUNIT_ASSERT(out == in[i]);
		CPPUNIT_ASSERT(msg.FindData("item", TypeCode, i,
					   (const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(*pout == in[i]);
		CPPUNIT_ASSERT(size == sizeof (Type));
	}
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy
>
TestSuite* 
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy>::
Suite()
{
	TestSuite* suite = new TestSuite("BMessage::Add/Find/Replace/HasRect()");

	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest1);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest2);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest3);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest4);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest5);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest6);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest7);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest8);

	return suite;
}
//------------------------------------------------------------------------------

#endif	// MESSAGEITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *

 */