/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include <JsonTextWriter.h>

#include <AutoDeleter.h>
#include <DataIO.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "JsonSamples.h"


using namespace BPrivate;


class JsonTextWriterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JsonTextWriterTest);
	CPPUNIT_TEST(TestString);
	CPPUNIT_TEST(TestFalse);
	CPPUNIT_TEST(TestDouble);
	CPPUNIT_TEST(TestInteger);
	CPPUNIT_TEST(TestObjectA);
	CPPUNIT_TEST(TestArrayA);
	CPPUNIT_TEST_SUITE_END();

	void _TestStringGeneric(const char *input, const char *expectedOut)
	{
		BMallocIO outputData;
		BJsonTextWriter writer(&outputData);

		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString(input));
		writer.Complete();

		BString outputString((char*)outputData.Buffer(),
			outputData.BufferLength());

		CPPUNIT_ASSERT_EQUAL(BString(expectedOut), outputString);
	}

public:
	void TestString()
	{
		_TestStringGeneric(
			"\"Eichh\xc3\xb6rnchen\"\nsind\nTiere.",
			"\"\\\"Eichh\\u00f6rnchen\\\"\\nsind\\nTiere.\"");
			// complex example with unicode, escapes and simple sequences.
		_TestStringGeneric("", "\"\"");
		_TestStringGeneric("Press \"C\" to continue",
			"\"Press \\\"C\\\" to continue\"");
			// test of a simple string of one character enclosed with escape
			// characters to check handling of one character simple sub-sequences.
		_TestStringGeneric("\xc3\xb6", "\"\\u00f6\"");
			// test of a unicode character on its own.
		_TestStringGeneric("simple", "\"simple\"");
			// test of a simple string that contains no escapes or anything complex.
		_TestStringGeneric("\t", "\"\\t\"");
			// test of a single escape character.
		_TestStringGeneric("\007B", "\"B\"");
			// contains an illegal character which should be ignored.
		_TestStringGeneric("X", "\"X\"");
			// a simple string with a single character
	}

	void TestFalse()
	{
		BMallocIO outputData;
		BJsonTextWriter writer(&outputData);
		static const char* expectedOut = "false";

		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteFalse());
		writer.Complete();

		BString outputString((char*)outputData.Buffer(),
			outputData.BufferLength());

		CPPUNIT_ASSERT_EQUAL(BString(expectedOut), outputString);
	}

	void TestDouble()
	{
		BMallocIO outputData;
		BJsonTextWriter writer(&outputData);
		static const char* expectedOut = "3.142857";

		CPPUNIT_ASSERT_EQUAL(B_OK,
			writer.WriteDouble(JSON_SAMPLE_NUMBER_A_LITERAL));
		writer.Complete();

		BString outputString((char *)outputData.Buffer(),
			outputData.BufferLength());

		CPPUNIT_ASSERT_EQUAL(BString(expectedOut), outputString);
	}

	void TestInteger()
	{
		BMallocIO outputData;
		BJsonTextWriter writer(&outputData);
		static const char* expectedOut = JSON_SAMPLE_NUMBER_B_EXPECTED_OUT;

		CPPUNIT_ASSERT_EQUAL(B_OK,
			writer.WriteInteger(JSON_SAMPLE_NUMBER_B_LITERAL));
		writer.Complete();

		BString outputString((char *)outputData.Buffer(),
			outputData.BufferLength());

		CPPUNIT_ASSERT_EQUAL(BString(expectedOut), outputString);
	}

	void TestObjectA()
	{
		BMallocIO outputData;
		BJsonTextWriter writer(&outputData);

		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteObjectStart());
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteObjectName("weather"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("raining"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteObjectName("humidity"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("too-high"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteObjectName("daysOfWeek"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteArrayStart());
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("MON"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("TUE"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("WED"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("THR"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("FRI"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteArrayEnd());
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteObjectEnd());
		writer.Complete();

		BString outputString((char*)outputData.Buffer(),
			outputData.BufferLength());

		CPPUNIT_ASSERT_EQUAL(BString(JSON_SAMPLE_OBJECT_A_EXPECTED_OUT),
			outputString);
	}

	void TestArrayA()
	{
		BMallocIO outputData;
		BJsonTextWriter writer(&outputData);

		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteArrayStart());
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("1234"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteInteger(4567));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteArrayStart());
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("A"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("b"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString("C\xc3\xa9zanne"));
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteArrayEnd());
		CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteArrayEnd());
		writer.Complete();

		BString outputString((char*)outputData.Buffer(),
			outputData.BufferLength());

		CPPUNIT_ASSERT_EQUAL(BString(JSON_SAMPLE_ARRAY_A_EXPECTED_OUT),
			outputString);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(JsonTextWriterTest, getTestSuiteName());
