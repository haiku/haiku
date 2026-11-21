/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageItemTest.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct Int8InitPolicy : public ArrayTypeBase<int8> {
	static int8 Zero() { return 0; }
	static int8 Test1() { return 16; }
	static int8 Test2() { return 32; }
	static size_t SizeOf(const int8&) { return sizeof(int8); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(64);
		array.push_back(128);
		array.push_back(255);
		return array;
	}
};


struct Int16InitPolicy : public ArrayTypeBase<int16> {
	static int16 Zero() { return 0; }
	static int16 Test1() { return 1234; }
	static int16 Test2() { return 5678; }
	static size_t SizeOf(const int16&) { return sizeof(int16); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(123);
		array.push_back(456);
		array.push_back(789);
		return array;
	}
};


struct Int32InitPolicy : public ArrayTypeBase<int32> {
	static int32 Zero() { return 0; }
	static int32 Test1() { return 1234; }
	static int32 Test2() { return 5678; }
	static size_t SizeOf(const int32&) { return sizeof(int32); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(123);
		array.push_back(456);
		array.push_back(789);
		return array;
	}
};


struct Int64InitPolicy : public ArrayTypeBase<int64> {
	static int64 Zero() { return 0; }
	static int64 Test1() { return 1234; }
	static int64 Test2() { return 5678; }
	static size_t SizeOf(const int64&) { return sizeof(int64); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(123);
		array.push_back(456);
		array.push_back(789);
		return array;
	}
};


typedef MessageItemAssertPolicy<int8> Int8AssertPolicy;
typedef MessageItemAssertPolicy<int16> Int16AssertPolicy;
typedef MessageItemAssertPolicy<int32> Int32AssertPolicy;
typedef MessageItemAssertPolicy<int64> Int64AssertPolicy;


typedef
MessageItemFuncPolicy<int8, &BMessage::AddInt8, &BMessage::FindInt8,
		&BMessage::FindInt8, &BMessage::FindInt8, &BMessage::HasInt8,
		&BMessage::ReplaceInt8>
	Int8FuncPolicy;
typedef
MessageItemFuncPolicy<int16, &BMessage::AddInt16, &BMessage::FindInt16,
		&BMessage::FindInt16, &BMessage::FindInt16,	&BMessage::HasInt16,
		&BMessage::ReplaceInt16>
	Int16FuncPolicy;
typedef
MessageItemFuncPolicy<int32, &BMessage::AddInt32, &BMessage::FindInt32,
		&BMessage::FindInt32, &BMessage::FindInt32, &BMessage::HasInt32,
		&BMessage::ReplaceInt32>
	Int32FuncPolicy;
typedef
MessageItemFuncPolicy<int64, &BMessage::AddInt64, &BMessage::FindInt64,
		&BMessage::FindInt64, &BMessage::FindInt64, &BMessage::HasInt64,
		&BMessage::ReplaceInt64>
	Int64FuncPolicy;


typedef
MessageItemTest<int8, B_INT8_TYPE, Int8FuncPolicy, Int8InitPolicy,
		Int8AssertPolicy>
	MessageInt8ItemTest;
typedef
MessageItemTest<int16, B_INT16_TYPE, Int16FuncPolicy, Int16InitPolicy,
		Int16AssertPolicy>
	MessageInt16ItemTest;
typedef
MessageItemTest<int32, B_INT32_TYPE, Int32FuncPolicy, Int32InitPolicy,
		Int32AssertPolicy>
	MessageInt32ItemTest;
typedef
MessageItemTest<int64, B_INT64_TYPE, Int64FuncPolicy, Int64InitPolicy,
		Int64AssertPolicy>
	MessageInt64ItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageInt8ItemTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageInt16ItemTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageInt32ItemTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageInt64ItemTest, getTestSuiteName());


typedef
MessageItemFuncPolicy<float, &BMessage::AddFloat, &BMessage::FindFloat,
		&BMessage::FindFloat, &BMessage::FindFloat, &BMessage::HasFloat,
		&BMessage::ReplaceFloat>
	FloatFuncPolicy;
typedef
MessageItemFuncPolicy<double, &BMessage::AddDouble,
		&BMessage::FindDouble, &BMessage::FindDouble, &BMessage::FindDouble,
		&BMessage::HasDouble, &BMessage::ReplaceDouble>
	DoubleFuncPolicy;


struct FloatInitPolicy : public ArrayTypeBase<float> {
	static float Zero() { return 0; }
	static float Test1() { return 1.234; }
	static float Test2() { return 5.678; }
	static size_t SizeOf(const float&) { return sizeof(float); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(1.23);
		array.push_back(4.56);
		array.push_back(7.89);
		return array;
	}
};


struct DoubleInitPolicy : public ArrayTypeBase<double> {
	static double Zero() { return 0; }
	static double Test1() { return 1.234; }
	static double Test2() { return 5.678; }
	static size_t SizeOf(const double&) { return sizeof(double); }
	static ArrayType Array()
	{
		ArrayType array;
		array.push_back(1.23);
		array.push_back(4.56);
		array.push_back(7.89);
		return array;
	}
};


struct FloatAssertPolicy {
	static float Zero() { return 0; }
	static float Invalid() { return 0; }
	static bool Size(size_t size, float& f) { return size == sizeof(f); }
};


struct DoubleAssertPolicy {
	static double Zero() { return 0; }
	static double Invalid() { return 0; }
	static bool Size(size_t size, double& d) { return size == sizeof(d); }
};


typedef
MessageItemTest<float, B_FLOAT_TYPE, FloatFuncPolicy, FloatInitPolicy,
		FloatAssertPolicy>
	MessageFloatItemTest;
typedef
MessageItemTest<double, B_DOUBLE_TYPE, DoubleFuncPolicy,
		DoubleInitPolicy, DoubleAssertPolicy>
	MessageDoubleItemTest;


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageFloatItemTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageDoubleItemTest, getTestSuiteName());
