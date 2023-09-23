/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "JwtTokenHelperTest.h"

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <string.h>

#include "JwtTokenHelper.h"


JwtTokenHelperTest::JwtTokenHelperTest()
{
}


JwtTokenHelperTest::~JwtTokenHelperTest()
{
}


void
JwtTokenHelperTest::TestParseTokenClaims()
{
	const char* jwtToken = "eyJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJkZXYuaGRzIiwic3ViIj"
		"oiZXJpazY0QGhkcyIsImV4cCI6MTY5MzkwNzM1NywiaWF0IjoxNjkzOTA3MDU3fQ.DJOz0"
		"TmgN0Ya8De-oV0mBwWb-8FYavLbaFUFhCLqr-s";
	BMessage actualMessage;

// ----------------------
	status_t result = JwtTokenHelper::ParseClaims(BString(jwtToken), actualMessage);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	_AssertStringValue(actualMessage, "iss", "dev.hds");
	_AssertStringValue(actualMessage, "sub", "erik64@hds");
	_AssertDoubleValue(actualMessage, "exp", 1693907357);
	_AssertDoubleValue(actualMessage, "iat", 1693907057);
}


void
JwtTokenHelperTest::TestCorrupt()
{
	const char* jwtToken = "application/json";
	BMessage actualMessage;

// ----------------------
	status_t result = JwtTokenHelper::ParseClaims(BString(jwtToken), actualMessage);
// ----------------------

	CPPUNIT_ASSERT(B_OK != result);
}


/*static*/ void
JwtTokenHelperTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("JwtTokenHelperTest");

	suite.addTest(
		new CppUnit::TestCaller<JwtTokenHelperTest>(
			"JwtTokenHelperTest::TestParseTokenClaims",
			&JwtTokenHelperTest::TestParseTokenClaims));

	suite.addTest(
		new CppUnit::TestCaller<JwtTokenHelperTest>(
			"JwtTokenHelperTest::TestCorrupt",
			&JwtTokenHelperTest::TestCorrupt));

	parent.addTest("JwtTokenHelperTest", &suite);
}


void
JwtTokenHelperTest::_AssertStringValue(const BMessage& message, const char* key,
	const char* expectedValue) const
{
	BString value;
	status_t result = message.FindString(key, &value);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(BString(expectedValue), value);
}


void
JwtTokenHelperTest::_AssertDoubleValue(const BMessage& message, const char* key,
	int64 expectedValue) const
{
	double value;
	status_t result = message.FindDouble(key, &value);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL((double) expectedValue, value);
}