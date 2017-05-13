/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#include "JsonToMessageTest.h"

#include <Json.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "JsonSamples.h"


#define LOOPS_TEST_OBJECT_A_FOR_PERFORMANCE 100000

using namespace BPrivate;


JsonToMessageTest::JsonToMessageTest()
{
}


JsonToMessageTest::~JsonToMessageTest()
{
}


void
JsonToMessageTest::TestArrayA()
{
	BMessage message;
	BMessage subMessage;
	BString stringValue;
	double doubleValue;

	// ----------------------
	BJson::Parse(JSON_SAMPLE_ARRAY_A_IN, message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [0]", B_OK,
		message.FindString("0", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [0]", BString("1234"), stringValue);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [1]", B_OK,
		message.FindDouble("1", &doubleValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [1]", 4567, doubleValue);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2]", B_OK,
		message.FindMessage("2", &subMessage));

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2.0]", B_OK,
		subMessage.FindString("0", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [2.0]", BString("A"), stringValue);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2.1]", B_OK,
		subMessage.FindString("1", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [2.1]", BString("b"), stringValue);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2.2]", B_OK,
		subMessage.FindString("2", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [2.2]", BString("C\xc3\xa9zanne"),
		stringValue);
}


void
JsonToMessageTest::TestArrayB()
{
	BMessage message;
	BMessage subMessage;
	BMessage subSubMessage;
	BString stringValue;

	// ----------------------
	BJson::Parse(JSON_SAMPLE_ARRAY_B_IN, message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [0]", B_OK,
		message.FindString("0", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [0]", BString("Whirinaki"), stringValue);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [1]", B_OK,
		message.FindString("1", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [1]", BString("Wellington"), stringValue);

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2]", B_OK,
		message.FindMessage("2", &subMessage));

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2.0]", B_OK,
		subMessage.FindMessage("key", &subSubMessage));

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!find [2.0.0]", B_OK,
		subSubMessage.FindString("0", &stringValue));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("!eq [2.0.0]", BString("value"), stringValue);
}


void
JsonToMessageTest::TestObjectA()
{
	BMessage message;
	BMessage subMessage;
	BString stringValue;

	// ----------------------
	BJson::Parse(JSON_SAMPLE_OBJECT_A_IN, message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindString("weather", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("raining"), stringValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindString("humidity", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("too-high"), stringValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("daysOfWeek", &subMessage));

	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindString("0", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("MON"), stringValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindString("1", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("TUE"), stringValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindString("2", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("WED"), stringValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindString("3", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("THR"), stringValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindString("4", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("FRI"), stringValue);
}


/*! This is not a real test, but is a convenient point at which to implement a
    performance test.
*/

void
JsonToMessageTest::TestObjectAForPerformance()
{
	int i;

	for (i=0; i < LOOPS_TEST_OBJECT_A_FOR_PERFORMANCE; i++)
		TestObjectA();
}


void
JsonToMessageTest::TestObjectB()
{
	BMessage message;
	bool boolValue;
	void *ptrValue; // this is how NULL is represented.

	// ----------------------
	BJson::Parse(JSON_SAMPLE_OBJECT_B_IN, message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindBool("testTrue", &boolValue));
	CPPUNIT_ASSERT_EQUAL(true, boolValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindBool("testFalse", &boolValue));
	CPPUNIT_ASSERT_EQUAL(false, boolValue);

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindPointer("testNull", &ptrValue));
	CPPUNIT_ASSERT_EQUAL(0, (addr_t)ptrValue);
}


void
JsonToMessageTest::TestObjectC()
{
	BMessage message;
	BMessage subMessage;
	BString stringValue;

	// ----------------------
	BJson::Parse(JSON_SAMPLE_OBJECT_C_IN, message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindString("key", &stringValue));
	CPPUNIT_ASSERT_EQUAL(BString("value"), stringValue);
}


void
JsonToMessageTest::TestUnterminatedObject()
{
	BMessage message;

	// ----------------------
	status_t result = BJson::Parse(JSON_SAMPLE_BROKEN_UNTERMINATED_OBJECT,
		message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_BAD_DATA, result);
}


void
JsonToMessageTest::TestUnterminatedArray()
{
	BMessage message;

	// ----------------------
	status_t result = BJson::Parse(JSON_SAMPLE_BROKEN_UNTERMINATED_ARRAY,
		message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_BAD_DATA, result);
}


void JsonToMessageTest::TestHaikuDepotFetchBatch()
{
	const unsigned char input[] = JSON_SAMPLE_HDS_FETCH_BATCH_PKGS;
	BMessage message;
	BMessage resultMessage;
	BMessage pkgsMessage;
	BMessage pkgMessage1;
	double modifyTimestampDouble;
	double expectedModifyTimestampDouble = 1488785331631.0;

	// ----------------------
	status_t result = BJson::Parse((char*)input,
		message);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);

		// this is quite a large test input so a random sample "thing to
		// check" is chosen to indicate that the parse was successful.

	CPPUNIT_ASSERT_EQUAL_MESSAGE("result", B_OK,
		message.FindMessage("result", &resultMessage));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("pkgs", B_OK,
		resultMessage.FindMessage("pkgs", &pkgsMessage));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("1", B_OK,
		pkgsMessage.FindMessage("1", &pkgMessage1));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("modifyTimestamp", B_OK,
		pkgMessage1.FindDouble("modifyTimestamp", &modifyTimestampDouble));

	CPPUNIT_ASSERT_EQUAL(expectedModifyTimestampDouble, modifyTimestampDouble);
}


/*static*/ void
JsonToMessageTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"JsonToMessageTest");

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestArrayA",
		&JsonToMessageTest::TestArrayA));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestArrayB",
		&JsonToMessageTest::TestArrayB));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestObjectA",
		&JsonToMessageTest::TestObjectA));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestObjectB",
		&JsonToMessageTest::TestObjectB));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestObjectC",
		&JsonToMessageTest::TestObjectC));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestUnterminatedObject",
		&JsonToMessageTest::TestUnterminatedObject));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestUnterminatedArray",
		&JsonToMessageTest::TestUnterminatedArray));

	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
		"JsonToMessageTest::TestHaikuDepotFetchBatch",
		&JsonToMessageTest::TestHaikuDepotFetchBatch));

//	suite.addTest(new CppUnit::TestCaller<JsonToMessageTest>(
//		"JsonToMessageTest::TestObjectAForPerformance",
//		&JsonToMessageTest::TestObjectAForPerformance));

	parent.addTest("JsonToMessageTest", &suite);
}
