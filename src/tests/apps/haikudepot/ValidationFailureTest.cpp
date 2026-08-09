/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Message.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "ValidationFailure.h"


class ValidationFailureTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ValidationFailureTest);
	CPPUNIT_TEST(Archive);
	CPPUNIT_TEST(Dearchive);
	CPPUNIT_TEST_SUITE_END();

	static void _FindValidationMessages(BMessage& validationFailureMessage,
		BStringList& validationMessages)
	{
		status_t result = B_OK;

		for (int32 i = 0; result == B_OK; i++) {
			BString validationMessage;
			BString name = "message_";
			name << i;
			result = validationFailureMessage.FindString(name,
				&validationMessage);

			if (result == B_OK) {
				validationMessages.Add(validationMessage);
			}
		}
	}

	static status_t
	_FindMessageWithProperty(const char* property, BMessage& validationFailuresMessage,
		BMessage& validationFailureMessage)
	{
		status_t result = B_OK;

		for (int32 i = 0; result == B_OK; i++) {
			BString name = "item_";
			name << i;
			result = validationFailuresMessage.FindMessage(name,
				&validationFailureMessage);

			if (result == B_OK) {
				BString messageProperty;
				result = validationFailureMessage.FindString("property",
					&messageProperty);

				if (result == B_OK && messageProperty == property)
					return result;
			}
		}

		return result;
	}

public:
	void Archive()
	{
		ValidationFailures failures;
		failures.AddFailure("nickname", "malformed");
		failures.AddFailure("nickname", "required");
		failures.AddFailure("passwordClear", "required");
		BMessage validationFailuresMessage;

		status_t archiveResult = failures.Archive(&validationFailuresMessage);

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Archive failure", B_OK, archiveResult);
		BMessage validationFailureNicknameMessage;
		BMessage validationFailurePasswordClearMessage;

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unable to find 'nickname'", B_OK,
			_FindMessageWithProperty("nickname", validationFailuresMessage,
				validationFailureNicknameMessage));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unable to find 'passwordClear'", B_OK,
			_FindMessageWithProperty("passwordClear", validationFailuresMessage,
				validationFailurePasswordClearMessage));

		BStringList validationFailureMessagesNickname;
		BStringList validationFailureMessagesPasswordClear;
		_FindValidationMessages(validationFailureNicknameMessage,
			validationFailureMessagesNickname);
		_FindValidationMessages(validationFailurePasswordClearMessage,
			validationFailureMessagesPasswordClear);

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unable to find 'nickname:malformed'",
			true, validationFailureMessagesNickname.HasString("malformed"));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unable to find 'nickname:required'",
			true, validationFailureMessagesNickname.HasString("required"));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unexpected validation messages 'nickname'",
			(int32) 2, validationFailureMessagesNickname.CountStrings());

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unable to find 'passwordClear:required'",
			true, validationFailureMessagesPasswordClear.HasString("required"));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Unexpected validation messages 'nickname'",
			(int32) 1, validationFailureMessagesPasswordClear.CountStrings());
	}

	void Dearchive()
	{
		BMessage nicknameMessage;
		nicknameMessage.AddString("property", "nickname");
		nicknameMessage.AddString("message_0", "malformed");
		nicknameMessage.AddString("message_1", "required");

		BMessage passwordClearMessage;
		passwordClearMessage.AddString("property", "passwordClear");
		passwordClearMessage.AddString("message_0", "required");

		BMessage validationFailuresMessage;
		validationFailuresMessage.AddMessage("item_0", &nicknameMessage);
		validationFailuresMessage.AddMessage("item_1", &passwordClearMessage);

		ValidationFailures validationFailures(&validationFailuresMessage);

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Count failures", (int32) 2,
			validationFailures.CountFailures());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Contains 'nickname'", true,
			validationFailures.Contains("nickname"));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Contains 'nickname:required'", true,
			validationFailures.Contains("nickname", "required"));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Contains 'nickname:malformed'", true,
			validationFailures.Contains("nickname", "malformed"));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!Contains 'passwordClear:required'", true,
			validationFailures.Contains("passwordClear", "required"));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ValidationFailureTest, getTestSuiteName());
