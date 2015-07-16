/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ConditionsTest.h"

#include <stdlib.h>

#include <driver_settings.h>
#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "Conditions.h"
#include "SettingsParser.h"


class TestConditionContext : public ConditionContext {
public:
			bool				IsSafeMode() const
									{ return false; }
			bool				BootVolumeIsReadOnly() const
									{ return false; }
};


static TestConditionContext sConditionContext;


ConditionsTest::ConditionsTest()
{
}


ConditionsTest::~ConditionsTest()
{
}


void
ConditionsTest::TestEmpty()
{
	Condition* condition = _Condition("");
	CPPUNIT_ASSERT(condition == NULL);
}


void
ConditionsTest::TestSafemode()
{
	Condition* condition = _Condition("safemode");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(condition->IsConstant(sConditionContext));

	class SafemodeConditionContext : public TestConditionContext {
	public:
		bool IsSafeMode() const
		{
			return true;
		}
	} safemodeContext;
	CPPUNIT_ASSERT(condition->Test(safemodeContext));
	CPPUNIT_ASSERT(condition->IsConstant(safemodeContext));
}


void
ConditionsTest::TestFileExists()
{
	Condition* condition = _Condition("file_exists /boot");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("file_exists /boot/don't fool me!");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
}


void
ConditionsTest::TestOr()
{
	Condition* condition = _Condition("or {\n"
		"file_exists /boot\n"
		"}\n");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("or {\n"
		"file_exists /nowhere\n"
		"}\n");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("or {\n"
		"file_exists /boot\n"
		"file_exists /nowhere\n"
		"}\n");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("or {\n"
		"not safemode\n"
		"file_exists /boot\n"
		"}\n");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));
	CPPUNIT_ASSERT(condition->IsConstant(sConditionContext));

	condition = _Condition("or {\n"
		"safemode\n"
		"file_exists /boot\n"
		"}\n");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));
}


void
ConditionsTest::TestAnd()
{
	Condition* condition = _Condition("and {\n"
		"file_exists /boot\n"
		"}\n");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("and {\n"
		"file_exists /nowhere\n"
		"}\n");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("and {\n"
		"file_exists /boot\n"
		"file_exists /nowhere\n"
		"}\n");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("and {\n"
		"safemode\n"
		"file_exists /nowhere\n"
		"}\n");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(condition->IsConstant(sConditionContext));

	condition = _Condition("and {\n"
		"not safemode\n"
		"file_exists /nowhere\n"
		"}\n");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

	condition = _Condition("and {\n"
		"safemode\n"
		"}\n");
	CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	CPPUNIT_ASSERT(condition->IsConstant(sConditionContext));
}


void
ConditionsTest::TestNot()
{
	Condition* condition = _Condition("not safemode");
	CPPUNIT_ASSERT(condition->Test(sConditionContext));

	class SafemodeConditionContext : public TestConditionContext {
	public:
		bool IsSafeMode() const
		{
			return true;
		}
	} safemodeContext;
	CPPUNIT_ASSERT(!condition->Test(safemodeContext));

}


/*static*/ void
ConditionsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("ConditionsTest");

	suite.addTest(new CppUnit::TestCaller<ConditionsTest>(
		"ConditionsTest::TestEmpty", &ConditionsTest::TestEmpty));
	suite.addTest(new CppUnit::TestCaller<ConditionsTest>(
		"ConditionsTest::TestSafemode", &ConditionsTest::TestSafemode));
	suite.addTest(new CppUnit::TestCaller<ConditionsTest>(
		"ConditionsTest::TestFileExists", &ConditionsTest::TestFileExists));
	suite.addTest(new CppUnit::TestCaller<ConditionsTest>(
		"ConditionsTest::TestOr", &ConditionsTest::TestOr));
	suite.addTest(new CppUnit::TestCaller<ConditionsTest>(
		"ConditionsTest::TestAnd", &ConditionsTest::TestAnd));
	suite.addTest(new CppUnit::TestCaller<ConditionsTest>(
		"ConditionsTest::TestNot", &ConditionsTest::TestNot));

	parent.addTest("ConditionsTest", &suite);
}


Condition*
ConditionsTest::_Condition(const char* string)
{
	SettingsParser parser;
	BString input("job A {\nif {");
	input << string << "\n}\n}\n";

	BMessage jobs;
	CPPUNIT_ASSERT_EQUAL(B_OK, parser.Parse(input, jobs));

	BMessage job;
	CPPUNIT_ASSERT_EQUAL(B_OK, jobs.FindMessage("job", 0, &job));
	CPPUNIT_ASSERT_EQUAL(2, job.CountNames(B_ANY_TYPE));
	CPPUNIT_ASSERT_EQUAL(BString("A"), BString(job.GetString("name")));

	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, job.FindMessage("if", &message));

	Condition* condition = Conditions::FromMessage(message);
	if (string[0] != '\0')
		CPPUNIT_ASSERT(condition != NULL);
	return condition;
}
