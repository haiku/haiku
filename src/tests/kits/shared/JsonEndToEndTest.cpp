/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#include "JsonEndToEndTest.h"

#include <AutoDeleter.h>

#include <Json.h>
#include <JsonTextWriter.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "JsonSamples.h"


using namespace BPrivate;


JsonEndToEndTest::JsonEndToEndTest()
{
}


JsonEndToEndTest::~JsonEndToEndTest()
{
}


void
JsonEndToEndTest::TestParseAndWrite(const char* input, const char* expectedOutput)
{
	BDataIO* inputData = new BMemoryIO(input, strlen(input));
	ObjectDeleter<BDataIO> inputDataDeleter(inputData);
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BPrivate::BJsonTextWriter* listener
		= new BJsonTextWriter(outputData);
	ObjectDeleter<BPrivate::BJsonTextWriter> listenerDeleter(listener);

// ----------------------
	BPrivate::BJson::Parse(inputData, listener);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, listener->ErrorStatus());
	fprintf(stderr, "in           >%s<\n", input);
	fprintf(stderr, "expected out >%s<\n", expectedOutput);
	fprintf(stderr, "actual out   >%s<\n", (char*)outputData->Buffer());
	CPPUNIT_ASSERT_MESSAGE("expected did no equal actual output",
		0 == strncmp(expectedOutput, (char*)outputData->Buffer(),
			strlen(expectedOutput)));
}


void
JsonEndToEndTest::TestNullA()
{
	TestParseAndWrite(JSON_SAMPLE_NULL_A_IN,
		JSON_SAMPLE_NULL_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestTrueA()
{
	TestParseAndWrite(JSON_SAMPLE_TRUE_A_IN,
		JSON_SAMPLE_TRUE_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestFalseA()
{
	TestParseAndWrite(JSON_SAMPLE_FALSE_A_IN,
		JSON_SAMPLE_FALSE_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestNumberA()
{
	TestParseAndWrite(JSON_SAMPLE_NUMBER_A_IN,
		JSON_SAMPLE_NUMBER_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestStringA()
{
	TestParseAndWrite(JSON_SAMPLE_STRING_A_IN,
		JSON_SAMPLE_STRING_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestStringB()
{
	TestParseAndWrite(JSON_SAMPLE_STRING_B_IN,
		JSON_SAMPLE_STRING_B_EXPECTED_OUT);
}


/* In this test, there are some UTF-8 characters. */

void
JsonEndToEndTest::TestStringA2()
{
	TestParseAndWrite(JSON_SAMPLE_STRING_A2_IN,
		JSON_SAMPLE_STRING_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestArrayA()
{
	TestParseAndWrite(JSON_SAMPLE_ARRAY_A_IN, JSON_SAMPLE_ARRAY_A_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestArrayB()
{
	TestParseAndWrite(JSON_SAMPLE_ARRAY_B_IN, JSON_SAMPLE_ARRAY_B_EXPECTED_OUT);
}


void
JsonEndToEndTest::TestObjectA()
{
	TestParseAndWrite(JSON_SAMPLE_OBJECT_A_IN,
		JSON_SAMPLE_OBJECT_A_EXPECTED_OUT);
}


/*! This method will test an element being unterminated; such an object that
    is missing the terminating "}" symbol or a string that has no closing
    quote.  This is tested here because the writer
*/

void
JsonEndToEndTest::TestUnterminated(const char *input)
{
	BDataIO* inputData = new BMemoryIO(input, strlen(input));
	ObjectDeleter<BDataIO> inputDataDeleter(inputData);
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BPrivate::BJsonTextWriter* listener
		= new BJsonTextWriter(outputData);
	ObjectDeleter<BPrivate::BJsonTextWriter> listenerDeleter(listener);

// ----------------------
	BPrivate::BJson::Parse(inputData, listener);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_BAD_DATA, listener->ErrorStatus());
}


void
JsonEndToEndTest::TestStringUnterminated()
{
	TestUnterminated(JSON_SAMPLE_BROKEN_UNTERMINATED_STRING);
}

void
JsonEndToEndTest::TestArrayUnterminated()
{
	TestUnterminated(JSON_SAMPLE_BROKEN_UNTERMINATED_ARRAY);
}

void
JsonEndToEndTest::TestObjectUnterminated()
{
	TestUnterminated(JSON_SAMPLE_BROKEN_UNTERMINATED_OBJECT);
}


/*static*/ void
JsonEndToEndTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("JsonEndToEndTest");

	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestNullA", &JsonEndToEndTest::TestNullA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestTrueA", &JsonEndToEndTest::TestTrueA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestFalseA", &JsonEndToEndTest::TestFalseA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestNumberA", &JsonEndToEndTest::TestNumberA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestStringA", &JsonEndToEndTest::TestStringA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestStringA2", &JsonEndToEndTest::TestStringA2));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestStringB", &JsonEndToEndTest::TestStringB));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestArrayA", &JsonEndToEndTest::TestArrayA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestArrayB", &JsonEndToEndTest::TestArrayB));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestObjectA", &JsonEndToEndTest::TestObjectA));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestStringUnterminated",
		&JsonEndToEndTest::TestStringUnterminated));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestArrayUnterminated",
		&JsonEndToEndTest::TestArrayUnterminated));
	suite.addTest(new CppUnit::TestCaller<JsonEndToEndTest>(
		"JsonEndToEndTest::TestObjectUnterminated",
		&JsonEndToEndTest::TestObjectUnterminated));

	parent.addTest("JsonEndToEndTest", &suite);
}
