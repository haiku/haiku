/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * Andrew Aldridge, i80and@foxquill.com
 */


#include <crypt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


#define PASSWORD "password"
#define HASH_SALT "$s$12$101f2cf1a3b35aa671b8e006c6fb037e429d5b4ecb8dab16919097789e2d3a5f$ignorethis"
#define HASH_RESULT "$s$12$101f2cf1a3b35aa671b8e006c6fb037e429d5b4ecb8dab16919097789e2d3a5f$4c5c886740871c447639e2dd5eeba004f22c0860ce88c811032ca6de6c95b23e"

// This salt is only 31 bytes, while we need 32 bytes
#define HASH_BAD_SALT "$s$12$101f2cf1a3b35aa671b8e006c6fb037e429d5b4ecb8dab16919097789e2d3a$ignorethis"

#define LEGACY_SALT "1d"
#define LEGACY_RESULT "1dVzQK99LSks6"

#define BSD_SALT "_7C/.Bf/4"
#define BSD_RESULT "_7C/.Bf/4gZk10RYRs4Y"


class CryptTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CryptTest);
	CPPUNIT_TEST(TestLegacy);
	CPPUNIT_TEST(TestLegacyBSD);
	CPPUNIT_TEST(TestCustomSalt);
	CPPUNIT_TEST(TestSaltGeneration);
	CPPUNIT_TEST(TestBadSalt);
	CPPUNIT_TEST(TestCryptR);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestLegacy()
	{
		char* buf = crypt(PASSWORD, LEGACY_SALT);
		CPPUNIT_ASSERT(buf != NULL);
		CPPUNIT_ASSERT(strcmp(buf, LEGACY_RESULT) == 0);
	}

	void TestLegacyBSD()
	{
		char* buf = crypt(PASSWORD, BSD_SALT);
		CPPUNIT_ASSERT(buf != NULL);
		CPPUNIT_ASSERT(strcmp(buf, BSD_RESULT) == 0);
	}

	void TestCustomSalt()
	{
		char* buf = crypt(PASSWORD, HASH_SALT);
		CPPUNIT_ASSERT(buf != NULL);
		CPPUNIT_ASSERT(strcmp(buf, HASH_RESULT) == 0);
	}

	void TestSaltGeneration()
	{
		char tmp[200];

		char* buf = crypt(PASSWORD, NULL);
		CPPUNIT_ASSERT(buf != NULL);
		strlcpy(tmp, buf, sizeof(tmp));
		buf = crypt(PASSWORD, tmp);
		CPPUNIT_ASSERT(strcmp(buf, tmp) == 0);
	}

	void TestBadSalt()
	{
		errno = 0;
		CPPUNIT_ASSERT(crypt(PASSWORD, HASH_BAD_SALT) == NULL);
		CPPUNIT_ASSERT(errno == EINVAL);
	}

	void TestCryptR()
	{
		char tmp[200];

		struct crypt_data data;
		data.initialized = 0;

		char* buf = crypt_r(PASSWORD, NULL, &data);
		CPPUNIT_ASSERT(buf != NULL);
		strlcpy(tmp, buf, sizeof(tmp));
		buf = crypt(PASSWORD, tmp);
		CPPUNIT_ASSERT(strcmp(buf, tmp) == 0);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CryptTest, getTestSuiteName());
