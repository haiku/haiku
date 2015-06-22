/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsParserTest.h"

#include <stdlib.h>

#include <driver_settings.h>
#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "SettingsParser.h"


SettingsParserTest::SettingsParserTest()
{
}


SettingsParserTest::~SettingsParserTest()
{
}


void
SettingsParserTest::TestConditionsMultiLine()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if {\n"
		"\tsafemode\n"
		"\tfile_exists one\n"
		"}\n", message));
	CPPUNIT_ASSERT_EQUAL(2, message.CountNames(B_ANY_TYPE));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("safemode", &subMessage));
	CPPUNIT_ASSERT(subMessage.IsEmpty());

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("file_exists",
		&subMessage));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(subMessage.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(1, subMessage.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestConditionsFlat()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if safemode\n", message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("safemode", &args));
	CPPUNIT_ASSERT(args.IsEmpty());
}


void
SettingsParserTest::TestConditionsFlatWithNot()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if not safemode\n", message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("not",
		&subMessage));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindMessage("safemode", &args));
	CPPUNIT_ASSERT(args.IsEmpty());
}


void
SettingsParserTest::TestConditionsFlatWithArgs()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK,
		_ParseCondition("if file_exists one\n", message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("file_exists", &args));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(args.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(1, args.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestConditionsFlatWithNotAndArgs()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK,
		_ParseCondition("if not file_exists one\n", message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("not",
		&subMessage));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindMessage("file_exists", &args));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(args.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(1, args.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestConditionsMultiLineFlatNot()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if {\n"
		"\tnot safemode\n"
		"}\n", message));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("not",
		&subMessage));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindMessage("safemode", &args));
	CPPUNIT_ASSERT_EQUAL(1, subMessage.CountNames(B_ANY_TYPE));
	CPPUNIT_ASSERT(args.IsEmpty());
}


void
SettingsParserTest::TestConditionsMultiLineFlatNotWithArgs()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if {\n"
		"\tnot file_exists one\n"
		"}\n", message));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("not",
		&subMessage));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindMessage("file_exists", &args));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(args.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(1, args.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestConditionsMultiLineNot()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if {\n"
		"\tnot {\n"
		"\t\tsafemode\n"
		"\t}\n"
		"}\n", message));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("not", &subMessage));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindMessage("safemode", &args));
	CPPUNIT_ASSERT(args.IsEmpty());

}


/*static*/ void
SettingsParserTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("SettingsParserTest");

	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsMultiLine",
		&SettingsParserTest::TestConditionsMultiLine));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsFlat",
		&SettingsParserTest::TestConditionsFlat));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsFlatWithNot",
		&SettingsParserTest::TestConditionsFlatWithNot));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsFlatWithArgs",
		&SettingsParserTest::TestConditionsFlatWithArgs));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsFlatWithNotAndArgs",
		&SettingsParserTest::TestConditionsFlatWithNotAndArgs));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsMultiLineFlatNot",
		&SettingsParserTest::TestConditionsMultiLineFlatNot));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsMultiLineFlatNotWithArgs",
		&SettingsParserTest::TestConditionsMultiLineFlatNotWithArgs));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsMultiLineNot",
		&SettingsParserTest::TestConditionsMultiLineNot));

	parent.addTest("SettingsParserTest", &suite);
}


status_t
SettingsParserTest::_ParseCondition(const char* text, BMessage& message)
{
	SettingsParser parser;
	BString input("job A {\n");
	input << text << "\n}\n";

	BMessage jobs;
	status_t status = parser.Parse(input, jobs);
	if (status != B_OK)
		return status;

	BMessage job;
	status = jobs.FindMessage("job", 0, &job);
	if (status != B_OK)
		return status;

	CPPUNIT_ASSERT_EQUAL(2, job.CountNames(B_ANY_TYPE));
	CPPUNIT_ASSERT_EQUAL(BString("A"), BString(job.GetString("name")));

	return job.FindMessage("if", &message);
}
