/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <String.h>
#include <string.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "DataIOUtils.h"


class DataIOUtilsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DataIOUtilsTest);
	CPPUNIT_TEST(ValidCode_Base64Read_ReturnsCorrectData);
	CPPUNIT_TEST(ValidCode_Base64Read_ReturnsCorrectData2);
	CPPUNIT_TEST(InvalidCharacter_Base64Read_ReturnsError);
	CPPUNIT_TEST_SUITE_END();

public:
	void InvalidCharacter_Base64Read_ReturnsError()
	{
		const char* jwtToken = "QW5k$mV3";
			// note that '$' is not a valid base64 character
		BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
		Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
		char actualOutputBuffer[7];
		size_t actualReadBytes;

		bzero(actualOutputBuffer, 7);

		status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 6, &actualReadBytes);

		CPPUNIT_ASSERT(B_OK != result);
	}

	void ValidCode_Base64Read_ReturnsCorrectData()
	{
		const char* jwtToken = "eyJpc3MiOiJkZXYuaGRzIiwic3ViIjoidG93ZWxkb3dudGVhQ"
			"GhkcyIsImV4cCI6MTY5MzczODgyNiwiaWF0IjoxNjkzNzM4NTI2fQ";
		BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
		Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
		char actualOutputBuffer[77];
		size_t actualReadBytes;

		bzero(actualOutputBuffer, 77);

		status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 76, &actualReadBytes);

		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(76, (int)actualReadBytes);
		actualOutputBuffer[actualReadBytes] = 0;

		CPPUNIT_ASSERT_EQUAL((uint8)0x7b, (uint8)actualOutputBuffer[0]);

		CPPUNIT_ASSERT_EQUAL(
			BString("{\"iss\":\"dev.hds\",\"sub\":\"toweldowntea@hds\",\"exp\":1693738826,\"iat\""
				":1693738526}"),
			BString(actualOutputBuffer));
	}

	void ValidCode_Base64Read_ReturnsCorrectData2()
	{
		const char* jwtToken = "eyJpc3MiOiJkZXYuaGRzIiwic3ViIjoiZXJpazY0QGhkcyIs"
			"ImV4cCI6MTY5MzE5MTMzMiwiaWF0IjoxNjkzMTkxMDMyfQ";
		BMemoryIO memoryIo(jwtToken, strlen(jwtToken));
		Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
		char actualOutputBuffer[71];
		size_t actualReadBytes;

		bzero(actualOutputBuffer, 71);

		status_t result = base64DecodingIo.ReadExactly(actualOutputBuffer, 70, &actualReadBytes);

		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(70, (int)actualReadBytes);
		actualOutputBuffer[actualReadBytes] = 0;

		CPPUNIT_ASSERT_EQUAL((uint8)0x7b, (uint8)actualOutputBuffer[0]);

		CPPUNIT_ASSERT_EQUAL(
			BString("{\"iss\":\"dev.hds\",\"sub\":\"erik64@hds\",\"exp\":1693191332,\"iat\""
				":1693191032}"),
			BString(actualOutputBuffer));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DataIOUtilsTest, getTestSuiteName());
