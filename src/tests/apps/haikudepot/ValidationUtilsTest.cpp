/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Message.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ValidationUtils.h"


class ValidationUtilsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ValidationUtilsTest);
	CPPUNIT_TEST(Email_Valid);
	CPPUNIT_TEST(Email_InvalidNoAt);
	CPPUNIT_TEST(Email_InvalidNoMailbox);
	CPPUNIT_TEST(Email_InvalidNoDomain);
	CPPUNIT_TEST(Nickname_Valid);
	CPPUNIT_TEST(Nickname_Invalid_Spaced);
	CPPUNIT_TEST(Nickname_Invalid_BadChars);
	CPPUNIT_TEST(PasswordClear_Valid);
	CPPUNIT_TEST(PasswordClear_Invalid_Weak);
	CPPUNIT_TEST_SUITE_END();

public:
	void PasswordClear_Invalid_Weak()
	{
		BString passwordClear("only has lower case letters");
		bool result = ValidationUtils::IsValidPasswordClear(passwordClear);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Password clear invalid", false, result);
	}

	void PasswordClear_Valid()
	{
		BString passwordClear("P4NhelQoad4");
		bool result = ValidationUtils::IsValidPasswordClear(passwordClear);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Password clear valid", true, result);
	}

	void Nickname_Invalid_BadChars()
	{
		BString nickname("erik!!10");
		bool result = ValidationUtils::IsValidNickname(nickname);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Nickname invalid (bad chars)",
			false, result);
	}

	void Nickname_Invalid_Spaced()
	{
		BString nickname("not a Nickname!");
		bool result = ValidationUtils::IsValidNickname(nickname);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Nickname invalid", false, result);
	}

	void Nickname_Valid()
	{
		BString nickname("erik55");
		bool result = ValidationUtils::IsValidNickname(nickname);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Nickname valid", true, result);
	}

	void Email_InvalidNoDomain()
	{
		BString email("fredric@");
		bool result = ValidationUtils::IsValidEmail(email);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email invalid - no domain", false, result);
	}

	void Email_InvalidNoMailbox()
	{
		BString email("@example.com");
		bool result = ValidationUtils::IsValidEmail(email);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email invalid - no mailbox", false, result);
	}

	void Email_InvalidNoAt()
	{
		BString email("wetaexample.com");
		bool result = ValidationUtils::IsValidEmail(email);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email invalid - no @", false, result);
	}

	void Email_Valid()
	{
		BString email("weta@example.com");
		bool result = ValidationUtils::IsValidEmail(email);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Email valid", true, result);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidationUtilsTest, getTestSuiteName());
