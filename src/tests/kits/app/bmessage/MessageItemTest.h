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
#include <typeinfo>
#include <posix/string.h>

// System Includes -------------------------------------------------------------
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------
#define TEMPLATE_TEST_PARAMS <Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>
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
							// size_t SizeOf(const Type&)
							// ArrayType Array()
							// typedef XXX ArrayType
	class AssertPolicy,		// Type Zero()
							// Type Invalid()
	class ComparePolicy		// bool Compare(const Type& lhs, const Type& rhs)
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
template<class Type>
struct TypePolicy
{
	enum { FixedSize = true };
	inline Type& Dereference(Type* p)
	{
		return *p;
	}
	inline Type* AddressOf(Type& t) { return &t; }
};
//------------------------------------------------------------------------------
template
<
	typename Type,
	status_t (BMessage::*AddFunc)(const char*, Type),
	status_t (BMessage::*FindFunc)(const char*, int32, Type*) const,
	status_t (BMessage::*ShortFindFunc)(const char*, Type*) const,
	Type (BMessage::*QuickFindFunc)(const char*, int32) const,
	bool (BMessage::*HasFunc)(const char*, int32) const,
	status_t (BMessage::*ReplaceFunc)(const char*, int32, Type),
	status_t (BMessage::*AddDataFunc)(const char*, type_code, const void*,
									  ssize_t, bool, int32) = &BMessage::AddData,
	status_t (BMessage::*FindDataFunc)(const char*, type_code, int32,
									   const void**, ssize_t*) const = &BMessage::FindData
>
struct TMessageItemFuncPolicy : public TypePolicy<Type>
{
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
//------------------------------------------------------------------------------
template<class T, T zero = T(), T invalid = T()>
struct TMessageItemAssertPolicy
{
	inline static T Zero()		{ return zero; }
	inline static T Invalid()	{ return invalid; }
	inline static bool Size(size_t size, T& t)
		{ return size == sizeof (t); }
};
//------------------------------------------------------------------------------
template<class T>
struct TMessageItemComparePolicy
{
	inline static bool Compare(const T& lhs, const T& rhs);
//		{ return lhs == rhs; }
};
template<class T>
bool 
TMessageItemComparePolicy<T>::Compare(const T &lhs, const T &rhs)
{
	 return lhs == rhs;
}
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
							// status_t FindData(BMessage&, const char*, type_code, int32, const void**, ssize_t*)
	class InitPolicy,		// Type Zero()
							// Type Test1()
							// Type Test2()
							// size_t SizeOf(const Type&)
							// ArrayType Array()
							// typedef XXX ArrayType
	class AssertPolicy,		// Type Zero()
							// Type Invalid()
							// bool Size(size_t, T&)
	class ComparePolicy		// bool Compare(const Type& lhs, const Type& rhs)
		= TMessageItemComparePolicy<Type>
>
class TMessageItemTest : public TestCase, public TypePolicy<Type>
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
		void MessageItemTest9();
		void MessageItemTest10();
		void MessageItemTest11();
		void MessageItemTest12();

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
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest1()
{
	BMessage msg;
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::ShortFind(msg, "item", &out) == B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_NAME_NOT_FOUND);
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
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest2()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
	CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0), in));
	CPPUNIT_ASSERT(FuncPolicy::ShortFind(msg, "item", &out) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(out, in));
	Type* pout = NULL;
	ssize_t size;
	status_t err = FuncPolicy::FindData(msg, "item", TypeCode, 0, 
										(const void**)&pout, &size);
	CPPUNIT_ASSERT(err == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in));
	CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest3()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type in2 = InitPolicy::Test2();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Replace(msg, "item", 0, in2) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
	CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0), in2));
	CPPUNIT_ASSERT(FuncPolicy::ShortFind(msg, "item", &out) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(out, in2));
	out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, 0, 
										(const void**)&pout, &size) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in2));
	CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest4()
{
	BMessage msg;
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 1, &out) == B_NAME_NOT_FOUND);
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
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest5()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	Type out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	
	for (uint32 ii = 0; ii < InitPolicy::Size(in); ++ii)
	{
		CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in[ii]) == B_OK);
	}

	for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", i),
											  in[i]));
		CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", i, &out) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in[i]));
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, i,
											(const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in[i]));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
	}
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest6()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	Type in2 = InitPolicy::Test2();
	Type out = InitPolicy::Zero();
	const int rIndex = 2;

	for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in[i]) == B_OK);
	}

	CPPUNIT_ASSERT(FuncPolicy::Replace(msg, "item", rIndex, in2) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", rIndex));
	CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", rIndex),
										  in2));
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", rIndex, &out) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(out, in2));
	out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, rIndex,
										(const void**)&pout, &size) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in2));
	CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest7()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::AddData(msg, "item", TypeCode, AddressOf(in),
									   InitPolicy::SizeOf(in),
									   TypePolicy<Type>::FixedSize) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", 0));
	CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", 0),
										  in));
	CPPUNIT_ASSERT(FuncPolicy::ShortFind(msg, "item", &out) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", 0, &out) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(out, in));
	Type* pout = NULL;
	ssize_t size;
	CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, 0, 
										(const void**)&pout, &size) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in));
	CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
}
//------------------------------------------------------------------------------
#include <stdio.h>
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest8()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	Type out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::AddData(msg, "item", TypeCode,
					   AddressOf(in[i]), InitPolicy::SizeOf(in[i]),
					   TypePolicy<Type>::FixedSize) == B_OK);
	}

	for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", i),
											  in[i]));
		CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", i, &out) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in[i]));
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, i,
											(const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in[i]));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
	}
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest9()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, NULL, in) == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void 
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest10()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::Has(msg, NULL, 0) == false);
	CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, NULL, 0),
										  AssertPolicy::Invalid()));
	CPPUNIT_ASSERT(FuncPolicy::ShortFind(msg, NULL, &out) == B_BAD_VALUE);
	CPPUNIT_ASSERT(FuncPolicy::Find(msg, NULL, 0, &out) == B_BAD_VALUE);
	Type* pout = NULL;
	ssize_t size;
	status_t err = FuncPolicy::FindData(msg, NULL, TypeCode, 0, 
										(const void**)&pout, &size);
	CPPUNIT_ASSERT(err == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void 
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest11()
{
	BMessage msg;
	Type in = InitPolicy::Test1();
	CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in) == B_OK);
	
	ssize_t flatSize = msg.FlattenedSize();
	char* buf = new char[flatSize];
	CPPUNIT_ASSERT(buf);

	CPPUNIT_ASSERT(msg.Flatten(buf, flatSize) == B_OK);
	
	BMessage msg2;
	Type out = InitPolicy::Zero();
	CPPUNIT_ASSERT(msg2.Unflatten(buf) == B_OK);
	CPPUNIT_ASSERT(FuncPolicy::ShortFind(msg, "item", &out) == B_OK);
	CPPUNIT_ASSERT(ComparePolicy::Compare(in, out));

	delete[] buf;
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
void 
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
MessageItemTest12()
{
	BMessage msg;
	ArrayType in = InitPolicy::Array();
	
	for (uint32 ii = 0; ii < InitPolicy::Size(in); ++ii)
	{
		CPPUNIT_ASSERT(FuncPolicy::Add(msg, "item", in[ii]) == B_OK);
	}
	
	ssize_t flatSize = msg.FlattenedSize();
	char* buf = new char[flatSize];
	CPPUNIT_ASSERT(buf);

	CPPUNIT_ASSERT(msg.Flatten(buf, flatSize) == B_OK);
	
	BMessage msg2;
	Type out = InitPolicy::Zero();
	Type* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg2.Unflatten(buf) == B_OK);

	for (uint32 i = 0; i < InitPolicy::Size(in); ++i)
	{
		CPPUNIT_ASSERT(FuncPolicy::Has(msg, "item", i));
		CPPUNIT_ASSERT(ComparePolicy::Compare(FuncPolicy::QuickFind(msg, "item", i),
											  in[i]));
		CPPUNIT_ASSERT(FuncPolicy::Find(msg, "item", i, &out) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(out, in[i]));
		CPPUNIT_ASSERT(FuncPolicy::FindData(msg, "item", TypeCode, i,
											(const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(ComparePolicy::Compare(Dereference(pout), in[i]));
		CPPUNIT_ASSERT(AssertPolicy::Size(size, Dereference(pout)));
	}

	delete[] buf;
}
//------------------------------------------------------------------------------
template
<
	class Type,
	type_code TypeCode,
	class FuncPolicy,
	class InitPolicy,
	class AssertPolicy,
	class ComparePolicy
>
TestSuite* 
TMessageItemTest<Type, TypeCode, FuncPolicy, InitPolicy, AssertPolicy, ComparePolicy>::
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
#ifndef TEST_R5
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest9);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest10);
#endif
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest11);
	ADD_TEMPLATE_TEST(BMessage, suite, TMessageItemTest TEMPLATE_TEST_PARAMS, MessageItemTest12);

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
