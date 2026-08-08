/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MediaFormats.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class MediaFormatsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MediaFormatsTest);
	CPPUNIT_TEST(Default_InitCheck_ReturnsOK);
	CPPUNIT_TEST(Default_FormatIteration_ReturnsOK);
	// CPPUNIT_TEST(DIVXFormat_GetCodeFor_ReturnsAVIFamily); // depends on installed codecs
	CPPUNIT_TEST_SUITE_END();

public:
	void Default_InitCheck_ReturnsOK()
	{
		BMediaFormats formats;
		CPPUNIT_ASSERT_EQUAL(B_OK, formats.InitCheck());
	}

	void Default_FormatIteration_ReturnsOK()
	{
		BMediaFormats formats;
		CPPUNIT_ASSERT_EQUAL(B_OK, formats.InitCheck());
	
		// Rewind() should only work when the formats object is locked
		status_t status = formats.RewindFormats();
		CPPUNIT_ASSERT(status != B_OK);
	
		CPPUNIT_ASSERT(formats.Lock() == true);
	
		status = formats.RewindFormats();
		CPPUNIT_ASSERT_EQUAL(B_OK, status);
	
		int32 count = 0;
		media_format format;
		media_format_description description;
		while ((status = formats.GetNextFormat(&format, &description)) == B_OK) {
			count++;
		}
		CPPUNIT_ASSERT_EQUAL(B_BAD_INDEX, status);
		CPPUNIT_ASSERT(count >= 0);
	
		formats.Unlock();
	}

	void DIVXFormat_GetCodeFor_ReturnsAVIFamily()
	{
		BMediaFormats formats;
		CPPUNIT_ASSERT_EQUAL(B_OK, formats.InitCheck());
	
		media_format format;
		status_t status = formats.GetAVIFormatFor('DIVX', &format, B_MEDIA_ENCODED_VIDEO);
		if (status == B_NAME_NOT_FOUND) {
			// This can happen if no DIVX codec is registered in the system
			return;
		}
		CPPUNIT_ASSERT_EQUAL(B_OK, status);
	
		media_format_description description;
		status = formats.GetCodeFor(format, B_AVI_FORMAT_FAMILY, &description);
		CPPUNIT_ASSERT_EQUAL(B_OK, status);

		status = formats.GetCodeFor(format, B_MPEG_FORMAT_FAMILY, &description);
		CPPUNIT_ASSERT(status != B_OK);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MediaFormatsTest, getTestSuiteName());
