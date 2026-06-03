/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <cstring>

#include <ByteOrder.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <TestSuiteAddon.h>

#include "convertutf.h"


// Tests based on "UTF-8 decoder capability and stress test"
// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
// Markus Kuhn - 2015-08-28 - CC BY 4.0

class ConvertUTFTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ConvertUTFTest);
	CPPUNIT_TEST(OneByteUTF8CharTest);
	CPPUNIT_TEST(TwoByteUTF8CharTest);
	CPPUNIT_TEST(ThreeByteUTF8CharTest);
	CPPUNIT_TEST(FourByteUTF8CharTest);
	CPPUNIT_TEST(FiveByteUTF8CharTest);
	CPPUNIT_TEST(SixByteUTF8CharTest);
	CPPUNIT_TEST(IncompleteConversionTest);
	CPPUNIT_TEST(OverflowParameterTest);
	CPPUNIT_TEST_SUITE_END();

public:
	void OneByteUTF8CharTest()
	{
		char utf8Input[2];
		utf8Input[1] = '\0';
		uint16 utf16Output[2];
		size_t sourceLength;
		ssize_t unitsWritten;

		// U-00000001
			// We never really convert U-00000000 (we halt conversion when we see that point),
			// so we test U-00000001 as the low-end one-byte character instead.
		utf8Input[0] = 1;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 1);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 1);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);
			// Two UTF-16 units should be written to the target buffer, including the
			// terminating NULL.

		// U-0000007F
		utf8Input[0] = 0x7F;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0x7F);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 1);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		return;
	}

	void TwoByteUTF8CharTest()
	{
		char utf8Input[3];
		utf8Input[2] = '\0';
		uint16 utf16Output[2];
		size_t sourceLength;
		ssize_t unitsWritten;

		// U-00000080
		utf8Input[0] = 0xC2;
		utf8Input[1] = 0x80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0x80);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 2);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-000007FF
		utf8Input[0] = 0xDF;
		utf8Input[1] = 0xBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0x7FF);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 2);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		return;
	}

	void ThreeByteUTF8CharTest()
	{
		char utf8Input[4];
		utf8Input[3] = '\0';
		uint16 utf16Output[2];
		size_t sourceLength;
		ssize_t unitsWritten;

		// U-00000800
		// Minimum 3-byte UTF-8 encoding
		utf8Input[0] = 0xE0;
		utf8Input[1] = 0xA0;
		utf8Input[2] = 0x80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0x800);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 3);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-0000D7FF
		// Maximum point below surrogate range
		utf8Input[0] = 0xED;
		utf8Input[1] = 0x9F;
		utf8Input[2] = 0xBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xD7FF);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 3);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-0000E000
		// Minimum point above surrogate range
		utf8Input[0] = 0xEE;
		utf8Input[1] = 0x80;
		utf8Input[2] = 0x80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xE000);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 3);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-0000FFFF
		// Maximum 3-byte UTF-8 encoding
		// Not a valid character - should be converted to the replacement character
		utf8Input[0] = 0xEF;
		utf8Input[1] = 0xBF;
		utf8Input[2] = 0xBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 3);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		return;
	}

	void FourByteUTF8CharTest()
	{
		char utf8Input[5];
		utf8Input[4] = '\0';
		uint16 utf16Output[3];
		size_t sourceLength;
		ssize_t unitsWritten;

		// U-00010000
		// Minimum four-byte UTF-8 encoding
		utf8Input[0] = 0xF0;
		utf8Input[1] = 0x90;
		utf8Input[2] = 0x80;
		utf8Input[3] = 0x80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);
		utf16Output[1] = B_LENDIAN_TO_HOST_INT16(utf16Output[1]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xD800);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0xDC00);
		CPPUNIT_ASSERT_EQUAL(utf16Output[2], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 4);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 3);

		// U-0010FFFF
		// Maximum Unicode character
		utf8Input[0] = 0xF4;
		utf8Input[1] = 0x8F;
		utf8Input[2] = 0xBF;
		utf8Input[3] = 0xBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);
		utf16Output[1] = B_LENDIAN_TO_HOST_INT16(utf16Output[1]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xDBFF);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0xDFFF);
		CPPUNIT_ASSERT_EQUAL(utf16Output[2], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 4);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 3);

		// U-00110000
		// Minimum point outside the range of Unicode and UTF-16 (but valid UTF-8).
		// This and higher points are converted to the Unicode special replacement character.
		utf8Input[0] = 0xF4;
		utf8Input[1] = 0x90;
		utf8Input[2] = 0x80;
		utf8Input[3] = 0x80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 4);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-001FFFFF
		// Maximum four-byte UTF-8 encoding
		utf8Input[0] = 0xF7;
		utf8Input[1] = 0xBF;
		utf8Input[2] = 0xBF;
		utf8Input[3] = 0xBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 4);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		return;
	}

	void FiveByteUTF8CharTest()
	{
		char utf8Input[6];
		utf8Input[5] = '\0';
		uint16 utf16Output[2];
		size_t sourceLength;
		ssize_t unitsWritten;

		// U-00200000
		utf8Input[0] = 0xF8;
		utf8Input[1] = 0x88;
		utf8Input[2] = 0x80;
		utf8Input[3] = 0x80;
		utf8Input[4] = 0x80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 5);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-03FFFFFF
		utf8Input[0] = 0xFB;
		utf8Input[1] = 0xBF;
		utf8Input[2] = 0xBF;
		utf8Input[3] = 0xBF;
		utf8Input[4] = 0xBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 5);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		return;
	}

	void SixByteUTF8CharTest()
	{
		char utf8Input[7];
		utf8Input[6] = '\0';
		uint16 utf16Output[2];
		size_t sourceLength;
		ssize_t unitsWritten;

		// U-04000000
		utf8Input[0] = 0xFC;
		utf8Input[1] = 0x84;
		utf8Input[2] = 0x80;
		utf8Input[3] = 0x80;
		utf8Input[4] = 0x80;
		utf8Input[5] = 0X80;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 6);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		// U-7FFFFFFF
		utf8Input[0] = 0xFD;
		utf8Input[1] = 0xBF;
		utf8Input[2] = 0xBF;
		utf8Input[3] = 0xBF;
		utf8Input[4] = 0xBF;
		utf8Input[5] = 0XBF;
		sourceLength = strlen(utf8Input);
		unitsWritten = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 2, NULL, true);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xFFFD);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 6);
		CPPUNIT_ASSERT_EQUAL(unitsWritten, 2);

		return;
	}

	void IncompleteConversionTest()
	{
		// Client requests null-teriminated output, meaning 4 output units would be needed for
		// full conversion, exceeding the output buffer size of 3 units.
		char utf8Input[7];
		utf8Input[0] = 0xCE;
		utf8Input[1] = 0x91;

		utf8Input[2] = 0xCE;
		utf8Input[3] = 0x91;

		utf8Input[4] = 0xCE;
		utf8Input[5] = 0x91;

		utf8Input[6] = '\0';
		size_t sourceLength = strlen(utf8Input);

		uint16 utf16Output1[3];
		uint16 overflow = 0;

		ssize_t written = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output1, 3, &overflow,
			true);
		utf16Output1[0] = B_LENDIAN_TO_HOST_INT16(utf16Output1[0]);
		utf16Output1[1] = B_LENDIAN_TO_HOST_INT16(utf16Output1[1]);

		CPPUNIT_ASSERT_EQUAL(utf16Output1[0], 0x0391);
		CPPUNIT_ASSERT_EQUAL(utf16Output1[1], 0x0391);
		CPPUNIT_ASSERT_EQUAL(utf16Output1[2], 0);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 4);
			// Ran out of space, could only convert 2 characters / 4 bytes.
		CPPUNIT_ASSERT_EQUAL(written, 3);
			// Two converted characters plus the requested NULL.
		CPPUNIT_ASSERT_EQUAL(overflow, 0);
			// This should be set to 0 unless we wrote half of a two-unit character.

		// Client does not request null-terminated output, so 3 output units are needed for
		// full conversion, exceeding the output buffer size of 2 units.
		sourceLength = strlen(utf8Input);
		uint16 utf16Output2[2];
		overflow = 0;

		written = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output2, 2, &overflow,
			false);
		utf16Output2[0] = B_LENDIAN_TO_HOST_INT16(utf16Output2[0]);
		utf16Output2[1] = B_LENDIAN_TO_HOST_INT16(utf16Output2[1]);

		CPPUNIT_ASSERT_EQUAL(utf16Output2[0], 0x0391);
		CPPUNIT_ASSERT_EQUAL(utf16Output2[1], 0x0391);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 4);
		CPPUNIT_ASSERT_EQUAL(written, 2);
		CPPUNIT_ASSERT_EQUAL(overflow, 0);

		return;
	}

	void OverflowParameterTest()
	{
		// The first two characters each have a two-unit representation in UTF-16.
		char utf8Input[10];
		utf8Input[0] = 0xF0;
		utf8Input[1] = 0x90;
		utf8Input[2] = 0x80;
		utf8Input[3] = 0x80;

		utf8Input[4] = 0xF0;
		utf8Input[5] = 0x90;
		utf8Input[6] = 0x80;
		utf8Input[7] = 0x80;

		utf8Input[8] = 0x41;

		utf8Input[9] = '\0';
		size_t sourceLength = strlen(utf8Input);

		// Target buffer has room for the first character and half of the second, if output
		// is not NULL-terminated.
		uint16 utf16Output[3];
		uint16 overflow = 0;

		ssize_t written = utf8_to_utf16le(utf8Input, &sourceLength, utf16Output, 3, &overflow,
			false);
		utf16Output[0] = B_LENDIAN_TO_HOST_INT16(utf16Output[0]);
		utf16Output[1] = B_LENDIAN_TO_HOST_INT16(utf16Output[1]);
		utf16Output[2] = B_LENDIAN_TO_HOST_INT16(utf16Output[2]);
		overflow = B_LENDIAN_TO_HOST_INT16(overflow);

		CPPUNIT_ASSERT_EQUAL(utf16Output[0], 0xD800);
		CPPUNIT_ASSERT_EQUAL(utf16Output[1], 0xDC00);
		CPPUNIT_ASSERT_EQUAL(utf16Output[2], 0xD800);
		CPPUNIT_ASSERT_EQUAL(overflow, 0xDC00);
		CPPUNIT_ASSERT_EQUAL(sourceLength, 8);
			// The first two characters were handled, but the third has yet to be converted.
		CPPUNIT_ASSERT_EQUAL(written, 3);
			// The unit written to the overflow variable should not be included in the returned
			// count.

		return;
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ConvertUTFTest, getTestSuiteName());
