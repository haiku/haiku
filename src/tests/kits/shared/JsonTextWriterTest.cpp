/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#include "JsonTextWriterTest.h"

#include <AutoDeleter.h>

#include <Json.h>
#include <JsonTextWriter.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "JsonSamples.h"


#define LOOPS_TEST_OBJECT_A_FOR_PERFORMANCE 100000


using namespace BPrivate;


JsonTextWriterTest::JsonTextWriterTest()
{
}


JsonTextWriterTest::~JsonTextWriterTest()
{
}


void
JsonTextWriterTest::TestArrayA()
{
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BJsonTextWriter writer(outputData);

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

	BString outputString((char*)outputData->Buffer(),
		outputData->BufferLength());
	fprintf(stderr, "expected out >%s<\n", JSON_SAMPLE_ARRAY_A_EXPECTED_OUT);
	fprintf(stderr, "actual out   >%s< (%" B_PRIuSIZE ")\n", outputString.String(),
		outputData->BufferLength());

	CPPUNIT_ASSERT_EQUAL(BString(JSON_SAMPLE_ARRAY_A_EXPECTED_OUT),
		outputString);
}


void
JsonTextWriterTest::TestObjectA()
{
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BJsonTextWriter writer(outputData);

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

	BString outputString((char*)outputData->Buffer(),
		outputData->BufferLength());
	fprintf(stderr, "expected out >%s<\n", JSON_SAMPLE_OBJECT_A_EXPECTED_OUT);
	fprintf(stderr, "actual out   >%s< (%" B_PRIuSIZE ")\n", outputString.String(),
		outputData->BufferLength());

	CPPUNIT_ASSERT_EQUAL(BString(JSON_SAMPLE_OBJECT_A_EXPECTED_OUT),
		outputString);
}


void
JsonTextWriterTest::TestObjectAForPerformance()
{
	int i;

	for (i = 0; i < LOOPS_TEST_OBJECT_A_FOR_PERFORMANCE; i++) {
		TestObjectA();
	}
}


void
JsonTextWriterTest::TestInteger()
{
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BJsonTextWriter writer(outputData);
	static const char* expectedOut = JSON_SAMPLE_NUMBER_B_EXPECTED_OUT;

	CPPUNIT_ASSERT_EQUAL(B_OK,
		writer.WriteInteger(JSON_SAMPLE_NUMBER_B_LITERAL));
	writer.Complete();

	BString outputString((char *)outputData->Buffer(),
		outputData->BufferLength());
	fprintf(stderr, "expected out >%s<\n", expectedOut);
	fprintf(stderr, "actual out   >%s< (%" B_PRIuSIZE ")\n", outputString.String(),
		outputData->BufferLength());

	CPPUNIT_ASSERT_EQUAL(BString(expectedOut),
		outputString);
}


void
JsonTextWriterTest::TestDouble()
{
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BJsonTextWriter writer(outputData);
	static const char* expectedOut = "3.142857";

	CPPUNIT_ASSERT_EQUAL(B_OK,
		writer.WriteDouble(JSON_SAMPLE_NUMBER_A_LITERAL));
	writer.Complete();

	BString outputString((char *)outputData->Buffer(),
		outputData->BufferLength());
	fprintf(stderr, "expected out >%s<\n", expectedOut);
	fprintf(stderr, "actual out   >%s< (%" B_PRIuSIZE ")\n", outputString.String(),
		outputData->BufferLength());

	CPPUNIT_ASSERT_EQUAL(BString(expectedOut),
		outputString);
}


void
JsonTextWriterTest::TestFalse()
{
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BJsonTextWriter writer(outputData);
	static const char* expectedOut = "false";

	CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteFalse());
	writer.Complete();

	BString outputString((char*)outputData->Buffer(),
		outputData->BufferLength());
	fprintf(stderr, "expected out >%s<\n", expectedOut);
	fprintf(stderr, "actual out   >%s< (%" B_PRIuSIZE ")\n",
		outputString.String(), outputData->BufferLength());

	CPPUNIT_ASSERT_EQUAL(BString(expectedOut),
		outputString);
}


void
JsonTextWriterTest::TestStringGeneric(const char *input,
	const char *expectedOut)
{
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
	BJsonTextWriter writer(outputData);

	CPPUNIT_ASSERT_EQUAL(B_OK, writer.WriteString(input));
	writer.Complete();

	BString outputString((char*)outputData->Buffer(),
		outputData->BufferLength());
	fprintf(stderr, "expected out >%s<\n", expectedOut);
	fprintf(stderr, "actual out   >%s< (%" B_PRIuSIZE ")\n",
		outputString.String(), outputData->BufferLength());

	CPPUNIT_ASSERT_EQUAL(BString(expectedOut),
		outputString);
}

void
JsonTextWriterTest::TestString()
{
	TestStringGeneric(
		"\"Eichh\xc3\xb6rnchen\"\nsind\nTiere.",
		"\"\\\"Eichh\\u00f6rnchen\\\"\\nsind\\nTiere.\"");
		// complex example with unicode, escapes and simple sequences.
	TestStringGeneric("", "\"\"");
	TestStringGeneric("Press \"C\" to continue",
		"\"Press \\\"C\\\" to continue\"");
		// test of a simple string of one character enclosed with escape
		// characters to check handling of one character simple sub-sequences.
	TestStringGeneric("\xc3\xb6", "\"\\u00f6\"");
		// test of a unicode character on its own.
	TestStringGeneric("simple", "\"simple\"");
		// test of a simple string that contains no escapes or anything complex.
	TestStringGeneric("\t", "\"\\t\"");
		// test of a single escape character.
	TestStringGeneric("\007B", "\"B\"");
		// contains an illegal character which should be ignored.
	TestStringGeneric("X", "\"X\"");
		// a simple string with a single character
}


/*static*/ void
JsonTextWriterTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"JsonTextWriterTest");

	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
		"JsonTextWriterTest::TestDouble",
		&JsonTextWriterTest::TestDouble));

	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
		"JsonTextWriterTest::TestInteger",
		&JsonTextWriterTest::TestInteger));

	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
		"JsonTextWriterTest::TestFalse",
		&JsonTextWriterTest::TestFalse));

	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
		"JsonTextWriterTest::TestString",
		&JsonTextWriterTest::TestString));

	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
		"JsonTextWriterTest::TestArrayA",
		&JsonTextWriterTest::TestArrayA));

	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
		"JsonTextWriterTest::TestObjectA",
		&JsonTextWriterTest::TestObjectA));

//	suite.addTest(new CppUnit::TestCaller<JsonTextWriterTest>(
//		"JsonTextWriterTest::TestObjectAForPerformance",
//		&JsonTextWriterTest::TestObjectAForPerformance));

	parent.addTest("JsonTextWriterTest", &suite);
}