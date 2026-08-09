/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Message.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "JwtTokenHelper.h"


class JwtTokenHelperTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JwtTokenHelperTest);
	CPPUNIT_TEST(ValidToken_ParseClaims_ReturnsSuccess);
	CPPUNIT_TEST(InvalidToken_ParseClaims_ReturnsError);
	CPPUNIT_TEST_SUITE_END();

	void _AssertDoubleValue(const BMessage& message, const char* key,
		int64 expectedValue) const
	{
		double value;
		status_t result = message.FindDouble(key, &value);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL((double)expectedValue, value);
	}

	void _AssertStringValue(const BMessage& message, const char* key,
		const char* expectedValue) const
	{
		BString value;
		status_t result = message.FindString(key, &value);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(expectedValue), value);
	}

public:
	void InvalidToken_ParseClaims_ReturnsError()
	{
		const char* jwtToken = "application/json";
		BMessage actualMessage;

		status_t result = JwtTokenHelper::ParseClaims(BString(jwtToken), actualMessage);

		CPPUNIT_ASSERT(B_OK != result);
	}

	void ValidToken_ParseClaims_ReturnsSuccess()
	{
		const char* jwtToken = "eyJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJkZXYuaGRzIiwic3ViIj"
			"oiZXJpazY0QGhkcyIsImV4cCI6MTY5MzkwNzM1NywiaWF0IjoxNjkzOTA3MDU3fQ.DJOz0"
			"TmgN0Ya8De-oV0mBwWb-8FYavLbaFUFhCLqr-s";
		BMessage actualMessage;

		status_t result = JwtTokenHelper::ParseClaims(BString(jwtToken), actualMessage);

		CPPUNIT_ASSERT_EQUAL(B_OK, result);

		_AssertStringValue(actualMessage, "iss", "dev.hds");
		_AssertStringValue(actualMessage, "sub", "erik64@hds");
		_AssertDoubleValue(actualMessage, "exp", 1693907357);
		_AssertDoubleValue(actualMessage, "iat", 1693907057);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(JwtTokenHelperTest, getTestSuiteName());
