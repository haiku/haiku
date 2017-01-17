/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * Andrew Aldridge, i80and@foxquill.com
 */


#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "CryptTest.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


#define PASSWORD "password"
#define HASH_SALT "$s$12$101f2cf1a3b35aa671b8e006c6fb037e429d5b4ecb8dab16919097789e2d3a5f$ignorethis"
#define HASH_RESULT "$s$12$101f2cf1a3b35aa671b8e006c6fb037e429d5b4ecb8dab16919097789e2d3a5f$4c5c886740871c447639e2dd5eeba004f22c0860ce88c811032ca6de6c95b23e"

// This salt is only 31 bytes, while we need 32 bytes
#define HASH_BAD_SALT "$s$12$101f2cf1a3b35aa671b8e006c6fb037e429d5b4ecb8dab16919097789e2d3a$ignorethis"


CryptTest::CryptTest()
{
}


CryptTest::~CryptTest()
{
}


void
CryptTest::setUp()
{
}


void
CryptTest::tearDown()
{
}


void
CryptTest::TestLegacy()
{
	char* buf = crypt(PASSWORD, "1d");
	CPPUNIT_ASSERT(buf != NULL);
	CPPUNIT_ASSERT(strcmp(buf, "1dVzQK99LSks6") == 0);
}


void
CryptTest::TestCustomSalt()
{
	char* buf = crypt(PASSWORD, HASH_SALT);
	CPPUNIT_ASSERT(buf != NULL);
	CPPUNIT_ASSERT(strcmp(buf, HASH_RESULT) == 0);
}


void
CryptTest::TestSaltGeneration()
{
	char tmp[200];

	char* buf = crypt(PASSWORD, NULL);
	CPPUNIT_ASSERT(buf != NULL);
	strlcpy(tmp, buf, sizeof(tmp));
	buf = crypt(PASSWORD, tmp);
	CPPUNIT_ASSERT(strcmp(buf, tmp) == 0);
}


void
CryptTest::TestBadSalt()
{
	errno = 0;
	CPPUNIT_ASSERT(crypt(PASSWORD, HASH_BAD_SALT) == NULL);
	CPPUNIT_ASSERT(errno == EINVAL);
}


void
CryptTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("CryptTest");
	suite.addTest(new CppUnit::TestCaller<CryptTest>(
		"CryptTest::TestLegacy",
		&CryptTest::TestLegacy));
	suite.addTest(new CppUnit::TestCaller<CryptTest>(
		"CryptTest::TestCustomSalt",
		&CryptTest::TestCustomSalt));
	suite.addTest(new CppUnit::TestCaller<CryptTest>(
		"CryptTest::TestSaltGeneration",
		&CryptTest::TestSaltGeneration));
	suite.addTest(new CppUnit::TestCaller<CryptTest>(
		"CryptTest::TestBadSalt",
		&CryptTest::TestBadSalt));
	parent.addTest("CryptTest", &suite);
}
