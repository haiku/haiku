/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ValidationUtilsTest.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "ValidationUtils.h"


ValidationUtilsTest::ValidationUtilsTest()
{
}


ValidationUtilsTest::~ValidationUtilsTest()
{
}


void
ValidationUtilsTest::TestEmailValid()
{
	BString email("weta@example.com");

// ----------------------
	bool result = ValidationUtils::IsValidEmail(email);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email valid", true, result);
}


			void				TestEmailInvalidNoAt();
			void				TestEmailInvalidNoMailbox();
			void				TestEmailInvalidNoDomain();
			void				TestEmailInvalidTwoAts();

void
ValidationUtilsTest::TestEmailInvalidNoAt()
{
	BString email("wetaexample.com");

// ----------------------
	bool result = ValidationUtils::IsValidEmail(email);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email invalid - no @", false, result);
}


void
ValidationUtilsTest::TestEmailInvalidNoMailbox()
{
	BString email("@example.com");

// ----------------------
	bool result = ValidationUtils::IsValidEmail(email);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email invalid - no mailbox", false, result);
}


void
ValidationUtilsTest::TestEmailInvalidNoDomain()
{
	BString email("fredric@");

// ----------------------
	bool result = ValidationUtils::IsValidEmail(email);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email invalid - no domain", false, result);
}


void
ValidationUtilsTest::TestNicknameValid()
{
	BString nickname("erik55");

// ----------------------
	bool result = ValidationUtils::IsValidNickname(nickname);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Nickname valid", true, result);
}


void
ValidationUtilsTest::TestNicknameInvalid()
{
	BString nickname("not a Nickname!");

// ----------------------
	bool result = ValidationUtils::IsValidNickname(nickname);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Nickname invalid", false, result);
}


void
ValidationUtilsTest::TestNicknameInvalidBadChars()
{
	BString nickname("erik!!10");

// ----------------------
	bool result = ValidationUtils::IsValidNickname(nickname);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Nickname invalid (bad chars)",
		false, result);
}


void
ValidationUtilsTest::TestPasswordClearValid()
{
	BString passwordClear("P4NhelQoad4");

// ----------------------
	bool result = ValidationUtils::IsValidPasswordClear(passwordClear);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Password clear valid", true, result);
}


void
ValidationUtilsTest::TestPasswordClearInvalid()
{
	BString passwordClear("only has lower case letters");
		// needs some numbers / upper case characters in there too

// ----------------------
	bool result = ValidationUtils::IsValidPasswordClear(passwordClear);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!Password clear invalid", false, result);
}


/*static*/ void
ValidationUtilsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"ValidationUtilsTest");

	suite.addTest(
		new CppUnit::TestCaller<ValidationUtilsTest>(
			"ValidationUtilsTest::TestEmailValid",
			&ValidationUtilsTest::TestEmailValid));
	suite.addTest(
		new CppUnit::TestCaller<ValidationUtilsTest>(
			"ValidationUtilsTest::TestNicknameInvalid",
			&ValidationUtilsTest::TestNicknameInvalid));
	suite.addTest(
		new CppUnit::TestCaller<ValidationUtilsTest>(
			"ValidationUtilsTest::TestNicknameValid",
			&ValidationUtilsTest::TestNicknameValid));
	suite.addTest(
		new CppUnit::TestCaller<ValidationUtilsTest>(
			"ValidationUtilsTest::TestNicknameInvalidBadChars",
			&ValidationUtilsTest::TestNicknameInvalidBadChars));
	suite.addTest(
		new CppUnit::TestCaller<ValidationUtilsTest>(
			"ValidationUtilsTest::TestPasswordClearInvalid",
			&ValidationUtilsTest::TestPasswordClearInvalid));
	suite.addTest(
		new CppUnit::TestCaller<ValidationUtilsTest>(
			"ValidationUtilsTest::TestPasswordClearValid",
			&ValidationUtilsTest::TestPasswordClearValid));

	parent.addTest("ValidationUtilsTest", &suite);
}
