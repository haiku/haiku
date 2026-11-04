/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <AutoDeleter.h>
#include <Json.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../../../apps/haikudepot/model/ValidationFailure.h"
#include "TestSchemaObjectModel.h"


/*!	This test will cover the code-generation correctness for JSON Schema models.
 */
class TestSchemaValidationTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TestSchemaValidationTest);

	CPPUNIT_TEST(TestValid);

	CPPUNIT_TEST(TestMissingValue);

	CPPUNIT_TEST(TestIntegerMinimum);
	CPPUNIT_TEST(TestIntegerMaximum);

	CPPUNIT_TEST(TestIntegerExclusiveMinimum);
	CPPUNIT_TEST(TestIntegerExclusiveMaximum);

	CPPUNIT_TEST(TestIntegerMultipleOf);

	CPPUNIT_TEST(TestStringMinLength);
	CPPUNIT_TEST(TestStringMaxLength);

	CPPUNIT_TEST_SUITE_END();

	static TestSchemaObjectRef _ValidExample()
	{
		return TestSchemaObjectBuilder()
			.WithName("Tane")
			.WithUsername("tane")
			.WithAge(45)
			// AgeDecade is missing
			.WithHeight(160)
			.BuildRef();
	}

public:
	void TestValid()
	{
		// GIVEN
		TestSchemaObjectRef ref = _ValidExample();

		// THEN
		ValidationFailures failuresName;
		status_t resultName = ref->IsValidName(&failuresName);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, resultName);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("name failures empty", true, failuresName.IsEmpty());

		ValidationFailures failuresUsername;
		status_t resultUsername = ref->IsValidUsername(&failuresUsername);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, resultUsername);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username failures empty", true, failuresUsername.IsEmpty());

		ValidationFailures failuresAge;
		status_t resultAge = ref->IsValidAge(&failuresAge);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, resultAge);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age failures empty", true, failuresAge.IsEmpty());

		ValidationFailures failuresAgeDecade;
		status_t resultAgeDecade = ref->IsValidAge(&failuresAgeDecade);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, resultAgeDecade);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade failures empty", true, failuresAgeDecade.IsEmpty());

		ValidationFailures failuresHeight;
		status_t resultHeight = ref->IsValidHeight(&failuresHeight);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, resultHeight);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("height failures empty", true, failuresHeight.IsEmpty());

		ValidationFailures failures;
		status_t result = ref->IsValid(&failures);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("object valid", B_OK, result);
		CPPUNIT_ASSERT_EQUAL_MESSAGE("object failures empty", true, failures.IsEmpty());
	}

	/*!	This is checking the Username is provided. If not then this is an error.
	 *	It also checks the overall validation fails in this case.
	 */
	void TestMissingValue()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithoutUsername()
			.BuildRef();

		// THEN

		status_t resultName = ref->IsValidName();
		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, resultName);

		{
			ValidationFailures failuresUsername;
			status_t resultUsername = ref->IsValidUsername(&failuresUsername);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_BAD_VALUE, resultUsername);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("username failures 1", 1, failuresUsername.CountFailures());
			ValidationFailure* failureUsername = failuresUsername.GetFailure("Username");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("username failure messages 1", 1,
				failureUsername->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("username failure message",
				BString("required but is missing"), failureUsername->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, ref->IsValidAge());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, ref->IsValidAgeDecade());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, ref->IsValidHeight());

		{
			ValidationFailures failures;
			status_t result = ref->IsValid(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("object failures 1", 1, failures.CountFailures());
			ValidationFailure* failureUsername = failures.GetFailure("Username");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("username failure messages 1", 1,
				failureUsername->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("username failure message",
				BString("required but is missing"), failureUsername->Messages().StringAt(0));
		}
	}

	void TestIntegerMinimum()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithAge(7) // <-- note
			.BuildRef();

		// THEN

		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, ref->IsValidName());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));

		{
			ValidationFailures failures;
			status_t result = ref->IsValidAge(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!age valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("Age");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age failure message",
				BString("7 less than 10"), failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, ref->IsValidHeight());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}

	void TestIntegerMaximum()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithAge(130) // <-- note
			.BuildRef();

		// THEN

		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, ref->IsValidName());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));

		{
			ValidationFailures failures;
			status_t result = ref->IsValidAge(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!age valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("Age");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age failure message",
				BString("130 greater than 120"), failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, ref->IsValidAgeDecade(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, ref->IsValidHeight());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}

	void TestIntegerExclusiveMinimum()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithHeight(4) // <-- note
			.BuildRef();

		// THEN

		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, ref->IsValidName());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, ref->IsValidAge(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, ref->IsValidAgeDecade(NULL));

		{
			ValidationFailures failures;
			status_t result = ref->IsValidHeight(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!height valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("height failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("Height");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("height failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("height failure message",
				BString("4 less than or equal 4"), failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}

	void TestIntegerExclusiveMaximum()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithHeight(200) // <-- note
			.BuildRef();

		// THEN

		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, ref->IsValidName());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, ref->IsValidAge(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, ref->IsValidAgeDecade(NULL));

		{
			ValidationFailures failures;
			status_t result = ref->IsValidHeight(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!height valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("height failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("Height");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("height failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("height failure message",
				BString("200 greater than or equal 200"), failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}

	void TestIntegerMultipleOf()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithAgeDecade(23)
			.BuildRef();

		// THEN

		CPPUNIT_ASSERT_EQUAL_MESSAGE("name valid", B_OK, ref->IsValidName());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, ref->IsValidAge(NULL));

		{
			ValidationFailures failures;
			status_t result = ref->IsValidAgeDecade(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!age decade valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("AgeDecade");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade failure message",
				BString("23 not multiple of 10"), failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, ref->IsValidHeight());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}

	void TestStringMinLength()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithName("A") // <--- too short
			.BuildRef();

		// THEN

		{
			ValidationFailures failures;
			status_t result = ref->IsValidName(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!name valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("name failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("Name");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("name failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("name failure message",
				BString("[A] (1) string length less than 2"), failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, ref->IsValidAge(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, ref->IsValidAgeDecade(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, ref->IsValidHeight());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}

	void TestStringMaxLength()
	{
		// GIVEN
		TestSchemaObjectRef ref = TestSchemaObjectBuilder(_ValidExample())
			.WithName("FlowerGrassTomatoCarrotConicalFig") // <--- too long
			.BuildRef();

		// THEN

		{
			ValidationFailures failures;
			status_t result = ref->IsValidName(&failures);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("!name valid", B_BAD_VALUE, result);
			CPPUNIT_ASSERT_EQUAL_MESSAGE("name failures 1", 1, failures.CountFailures());
			ValidationFailure* failure = failures.GetFailure("Name");
			CPPUNIT_ASSERT_EQUAL_MESSAGE("name failure messages 1", 1,
				failure->Messages().CountStrings());
			CPPUNIT_ASSERT_EQUAL_MESSAGE("name failure message",
				BString("[FlowerGrassTomatoCarrotConicalFig] (33) string length greater than 16"),
				failure->Messages().StringAt(0));
		}

		CPPUNIT_ASSERT_EQUAL_MESSAGE("username valid", B_OK, ref->IsValidUsername(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age valid", B_OK, ref->IsValidAge(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("age decade valid", B_OK, ref->IsValidAgeDecade(NULL));
		CPPUNIT_ASSERT_EQUAL_MESSAGE("height valid", B_OK, ref->IsValidHeight());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!object valid", B_BAD_VALUE, ref->IsValid(NULL));
	}
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TestSchemaValidationTest, getTestSuiteName());
