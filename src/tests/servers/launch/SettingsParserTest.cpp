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


// #pragma mark - conditions


void
SettingsParserTest::TestConditionsEmpty()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseCondition("if {\n"
		"}\n", message));
	CPPUNIT_ASSERT(message.IsEmpty());
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
		"\tnot file_exists one two\n"
		"}\n", message));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("not",
		&subMessage));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, subMessage.FindMessage("file_exists", &args));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(args.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(BString("two"),
		BString(args.GetString("args", 1, "-")));
	CPPUNIT_ASSERT_EQUAL(1, args.CountNames(B_ANY_TYPE));
	CPPUNIT_ASSERT_EQUAL(2, _ArrayCount(args, "args"));
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


// #pragma mark - events


void
SettingsParserTest::TestEventsEmpty()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseEvent("on {\n"
		"}\n", message));
	CPPUNIT_ASSERT(message.IsEmpty());
}


void
SettingsParserTest::TestEventsMultiLine()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseEvent("on {\n"
		"\tfile_created one\n"
		"\tdemand\n"
		"}\n", message));
	CPPUNIT_ASSERT_EQUAL(2, message.CountNames(B_ANY_TYPE));

	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("demand", &subMessage));
	CPPUNIT_ASSERT(subMessage.IsEmpty());

	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("file_created",
		&subMessage));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(subMessage.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(1, subMessage.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestEventsFlat()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseEvent("on demand\n", message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("demand", &args));
	CPPUNIT_ASSERT(args.IsEmpty());
}


void
SettingsParserTest::TestEventsFlatWithArgs()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseEvent("on file_created one\n", message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	BMessage args;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("file_created", &args));
	CPPUNIT_ASSERT_EQUAL(BString("one"),
		BString(args.GetString("args", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(1, args.CountNames(B_ANY_TYPE));
}


// #pragma mark - environment


void
SettingsParserTest::TestEnvironmentMultiLine()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseName("env", "env {\n"
		"from_script SetupEnvironment\n"
		"TEST well, yes\n"
		"}\n", message));
	CPPUNIT_ASSERT_EQUAL(2, message.CountNames(B_ANY_TYPE));

	CPPUNIT_ASSERT_EQUAL(BString("SetupEnvironment"),
		BString(message.GetString("from_script", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(message, "from_script"));

	CPPUNIT_ASSERT_EQUAL(BString("well,"),
		BString(message.GetString("TEST", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(BString("yes"),
		BString(message.GetString("TEST", 1, "-")));
	CPPUNIT_ASSERT_EQUAL(2, _ArrayCount(message, "TEST"));
}


void
SettingsParserTest::TestEnvironmentFlat()
{
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, _ParseName("env", "env SetupEnvironment\n",
		message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	CPPUNIT_ASSERT_EQUAL(BString("SetupEnvironment"),
		BString(message.GetString("from_script", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(message, "from_script"));
}


// #pragma mark - run


void
SettingsParserTest::TestRunFlat()
{
	SettingsParser parser;
	BMessage jobs;
	CPPUNIT_ASSERT_EQUAL(B_OK, parser.Parse("run me", jobs));
	CPPUNIT_ASSERT_EQUAL(1, jobs.CountNames(B_ANY_TYPE));

	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, jobs.FindMessage("run", &message));
	CPPUNIT_ASSERT_EQUAL(BString("me"),
		BString(message.GetString("target", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(message, "target"));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestRunMultiLine()
{
	SettingsParser parser;
	BMessage jobs;
	status_t status = parser.Parse("run {\n"
		"\tme\n"
		"\tyou\n"
		"}\n", jobs);
	CPPUNIT_ASSERT_EQUAL(B_OK, status);
	CPPUNIT_ASSERT_EQUAL(1, jobs.CountNames(B_ANY_TYPE));

	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, jobs.FindMessage("run", &message));
	CPPUNIT_ASSERT_EQUAL(BString("me"),
		BString(message.GetString("target", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(BString("you"),
		BString(message.GetString("target", 1, "-")));
	CPPUNIT_ASSERT_EQUAL(2, _ArrayCount(message, "target"));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestRunIfThenElseFlat()
{
	SettingsParser parser;
	BMessage jobs;
	status_t status = parser.Parse("run {\n"
		"\tif safemode\n"
		"\tthen this\n"
		"\telse that\n"
		"}\n", jobs);
	CPPUNIT_ASSERT_EQUAL(B_OK, status);

	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, jobs.FindMessage("run", &message));
	CPPUNIT_ASSERT_EQUAL(3, message.CountNames(B_ANY_TYPE));

	BMessage then;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("then", &then));
	CPPUNIT_ASSERT_EQUAL(BString("this"),
		BString(then.GetString("target", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(then, "target"));
	CPPUNIT_ASSERT_EQUAL(1, then.CountNames(B_ANY_TYPE));

	BMessage otherwise;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("else", &otherwise));
	CPPUNIT_ASSERT_EQUAL(BString("that"),
		BString(otherwise.GetString("target", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(otherwise, "target"));
	CPPUNIT_ASSERT_EQUAL(1, otherwise.CountNames(B_ANY_TYPE));
}


void
SettingsParserTest::TestRunIfThenElseMultiLine()
{
	SettingsParser parser;
	BMessage jobs;
	status_t status = parser.Parse("run {\n"
		"\tif {\n"
		"\t\tread_only\n"
		"\t}\n"
		"\tthen {\n"
		"\t\tthis\n"
		"\t}\n"
		"\telse {\n"
		"\t\tthat\n"
		"\t}\n"
		"}\n", jobs);
	CPPUNIT_ASSERT_EQUAL(B_OK, status);

	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, jobs.FindMessage("run", &message));
	CPPUNIT_ASSERT_EQUAL(3, message.CountNames(B_ANY_TYPE));

	BMessage then;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("then", &then));
	CPPUNIT_ASSERT_EQUAL(BString("this"),
		BString(then.GetString("target", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(then, "target"));
	CPPUNIT_ASSERT_EQUAL(1, then.CountNames(B_ANY_TYPE));

	BMessage otherwise;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("else", &otherwise));
	CPPUNIT_ASSERT_EQUAL(BString("that"),
		BString(otherwise.GetString("target", "-")));
	CPPUNIT_ASSERT_EQUAL(1, _ArrayCount(otherwise, "target"));
	CPPUNIT_ASSERT_EQUAL(1, otherwise.CountNames(B_ANY_TYPE));
}


// #pragma mark -


/*static*/ void
SettingsParserTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("SettingsParserTest");

	// Conditions
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestConditionsEmpty",
		&SettingsParserTest::TestConditionsEmpty));
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

	// Events
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestEventsEmpty",
		&SettingsParserTest::TestEventsEmpty));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestEventsMultiLine",
		&SettingsParserTest::TestEventsMultiLine));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestEventsFlat",
		&SettingsParserTest::TestEventsFlat));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestEventsFlatWithArgs",
		&SettingsParserTest::TestEventsFlatWithArgs));

	// Environment
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestEnvironmentMultiLine",
		&SettingsParserTest::TestEnvironmentMultiLine));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestEnvironmentFlat",
		&SettingsParserTest::TestEnvironmentFlat));

	// Run
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestRunFlat",
		&SettingsParserTest::TestRunFlat));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestRunMultiLine",
		&SettingsParserTest::TestRunMultiLine));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestRunIfThenElseFlat",
		&SettingsParserTest::TestRunIfThenElseFlat));
	suite.addTest(new CppUnit::TestCaller<SettingsParserTest>(
		"SettingsParserTest::TestRunIfThenElseMultiLine",
		&SettingsParserTest::TestRunIfThenElseMultiLine));

	parent.addTest("SettingsParserTest", &suite);
}


status_t
SettingsParserTest::_ParseCondition(const char* text, BMessage& message)
{
	return _ParseName("if", text, message);
}


status_t
SettingsParserTest::_ParseEvent(const char* text, BMessage& message)
{
	return _ParseName("on", text, message);
}


status_t
SettingsParserTest::_ParseName(const char* name, const char* text,
	BMessage& message)
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

	return job.FindMessage(name, &message);
}


int32
SettingsParserTest::_ArrayCount(BMessage& message, const char* name)
{
	int32 found;
	if (message.GetInfo(name, NULL, &found, NULL) != B_OK)
		return 0;

	return found;
}
