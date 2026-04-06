/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>

#include <driver_settings.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

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


class ConditionsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ConditionsTest);
	CPPUNIT_TEST(TestEmpty);
	CPPUNIT_TEST(TestSafemode);
	CPPUNIT_TEST(TestFileExists);
	CPPUNIT_TEST(TestOr);
	CPPUNIT_TEST(TestAnd);
	CPPUNIT_TEST(TestNot);
	CPPUNIT_TEST_SUITE_END();

	Condition* _Condition(const char* string)
	{
		SettingsParser parser;
		BString input("job A {\nif {");
		input << string << "\n}\n}\n";

		BMessage jobs;
		CPPUNIT_ASSERT_EQUAL(B_OK, parser.Parse(input, jobs));

		BMessage job;
		CPPUNIT_ASSERT_EQUAL(B_OK, jobs.FindMessage("job", 0, &job));
		CPPUNIT_ASSERT_EQUAL(2, (int)job.CountNames(B_ANY_TYPE));
		CPPUNIT_ASSERT_EQUAL(BString("A"), BString(job.GetString("name")));

		BMessage message;
		CPPUNIT_ASSERT_EQUAL(B_OK, job.FindMessage("if", &message));

		Condition* condition = Conditions::FromMessage(message);
		if (string[0] != '\0')
			CPPUNIT_ASSERT(condition != NULL);
		return condition;
	}

	class SafemodeConditionContext : public TestConditionContext {
	public:
		bool IsSafeMode() const
		{
			return true;
		}
	};

public:
	void TestEmpty()
	{
		Condition* condition = _Condition("");
		CPPUNIT_ASSERT(condition == NULL);
	}

	void TestSafemode()
	{
		Condition* condition = _Condition("safemode");
		CPPUNIT_ASSERT(!condition->Test(sConditionContext));
		CPPUNIT_ASSERT(condition->IsConstant(sConditionContext));

		SafemodeConditionContext safemodeContext;
		CPPUNIT_ASSERT(condition->Test(safemodeContext));
		CPPUNIT_ASSERT(condition->IsConstant(safemodeContext));
	}

	void TestFileExists()
	{
		Condition* condition = _Condition("file_exists /boot");
		CPPUNIT_ASSERT(condition->Test(sConditionContext));
		CPPUNIT_ASSERT(!condition->IsConstant(sConditionContext));

		condition = _Condition("file_exists /boot/don't fool me!");
		CPPUNIT_ASSERT(!condition->Test(sConditionContext));
	}

	void TestOr()
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

	void TestAnd()
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

	void TestNot()
	{
		Condition* condition = _Condition("not safemode");
		CPPUNIT_ASSERT(condition->Test(sConditionContext));

		SafemodeConditionContext safemodeContext;
		CPPUNIT_ASSERT(!condition->Test(safemodeContext));

	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ConditionsTest, getTestSuiteName());
