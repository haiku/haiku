/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DataIOUtilsTest.h"

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <string.h>

#include "DataIOUtils.h"


DataIOUtilsTest::DataIOUtilsTest()
{
}


DataIOUtilsTest::~DataIOUtilsTest()
{
}


void
DataIOUtilsTest::TestReadBase64JwtClaims_1()
{
	const char* jwtToken = "eyJpc3MiOiJkZXYuaGRzIiwic3ViIjoiZXJpazY0QGhkcyIs"
		"ImV4cCI6MTY5MzE5MTMzMiwiaWF0IjoxNjkzMTkxMDMyfQ";
	BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	char actualOutputBuffer[71];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 71);

// ----------------------
	status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 70, &actualReadBytes);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(70, actualReadBytes);
	actualOutputBuffer[actualReadBytes] = 0;

	CPPUNIT_ASSERT_EQUAL(0x7b, (uint8) actualOutputBuffer[0]);

	CPPUNIT_ASSERT_EQUAL(
		BString("{\"iss\":\"dev.hds\",\"sub\":\"erik64@hds\",\"exp\":1693191332,\"iat\""
			":1693191032}"),
		BString(actualOutputBuffer));
}


void
DataIOUtilsTest::TestReadBase64JwtClaims_2()
{
	const char* jwtToken = "eyJpc3MiOiJkZXYuaGRzIiwic3ViIjoidG93ZWxkb3dudGVhQ"
		"GhkcyIsImV4cCI6MTY5MzczODgyNiwiaWF0IjoxNjkzNzM4NTI2fQ";
	BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	char actualOutputBuffer[77];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 77);

// ----------------------
	status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 76, &actualReadBytes);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(76, actualReadBytes);
	actualOutputBuffer[actualReadBytes] = 0;

	CPPUNIT_ASSERT_EQUAL(0x7b, (uint8) actualOutputBuffer[0]);

	CPPUNIT_ASSERT_EQUAL(
		BString("{\"iss\":\"dev.hds\",\"sub\":\"toweldowntea@hds\",\"exp\":1693738826,\"iat\""
			":1693738526}"),
		BString(actualOutputBuffer));
}


void
DataIOUtilsTest::TestCorrupt()
{
	const char* jwtToken = "QW5k$mV3";
		// note that '$' is not a valid base64 character
	BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	char actualOutputBuffer[7];
	size_t actualReadBytes;

	bzero(actualOutputBuffer, 7);

// ----------------------
	status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 6, &actualReadBytes);
// ----------------------

	CPPUNIT_ASSERT(B_OK != result);
}


/*static*/ void
DataIOUtilsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DataIOUtilsTest");

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestReadBase64JwtClaims_1",
			&DataIOUtilsTest::TestReadBase64JwtClaims_1));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestReadBase64JwtClaims_2",
			&DataIOUtilsTest::TestReadBase64JwtClaims_2));

	suite.addTest(
		new CppUnit::TestCaller<DataIOUtilsTest>(
			"DataIOUtilsTest::TestCorrupt",
			&DataIOUtilsTest::TestCorrupt));

	parent.addTest("DataIOUtilsTest", &suite);
}