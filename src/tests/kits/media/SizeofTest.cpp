/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <MediaKit.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class SizeofTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SizeofTest);
	CPPUNIT_TEST(API32Bit_Sizeof_ReturnsExpected);
	CPPUNIT_TEST_SUITE_END();

public:
	void API32Bit_Sizeof_ReturnsExpected()
	{
#ifdef B_HAIKU_32_BIT
		CPPUNIT_ASSERT_EQUAL(264, (int)sizeof(BBuffer));
		CPPUNIT_ASSERT_EQUAL(240, (int)sizeof(BBufferConsumer));
		CPPUNIT_ASSERT_EQUAL(56, (int)sizeof(BBufferGroup));
		CPPUNIT_ASSERT_EQUAL(244, (int)sizeof(BBufferProducer));
		CPPUNIT_ASSERT_EQUAL(140, (int)sizeof(BContinuousParameter));
		CPPUNIT_ASSERT_EQUAL(240, (int)sizeof(BControllable));
		CPPUNIT_ASSERT_EQUAL(124, (int)sizeof(BDiscreteParameter));
		CPPUNIT_ASSERT_EQUAL(236, (int)sizeof(BFileInterface));
		CPPUNIT_ASSERT_EQUAL(40, (int)sizeof(BMediaAddOn));
		CPPUNIT_ASSERT_EQUAL(340, (int)sizeof(BMediaEventLooper));
		CPPUNIT_ASSERT_EQUAL(560, (int)sizeof(BMediaFile));
		CPPUNIT_ASSERT_EQUAL(72, (int)sizeof(BMediaFiles));
		CPPUNIT_ASSERT_EQUAL(128, (int)sizeof(BMediaFormats));
		CPPUNIT_ASSERT_EQUAL(164, (int)sizeof(BMediaNode));
		CPPUNIT_ASSERT_EQUAL(440, (int)sizeof(BMediaRoster));
		CPPUNIT_ASSERT_EQUAL(68, (int)sizeof(BMediaTheme));
		CPPUNIT_ASSERT_EQUAL(760, (int)sizeof(BMediaTrack));
		CPPUNIT_ASSERT_EQUAL(116, (int)sizeof(BNullParameter));
		CPPUNIT_ASSERT_EQUAL(84, (int)sizeof(BParameter));
		CPPUNIT_ASSERT_EQUAL(52, (int)sizeof(BParameterGroup));
		CPPUNIT_ASSERT_EQUAL(124, (int)sizeof(BSound));
		CPPUNIT_ASSERT_EQUAL(164, (int)sizeof(BTimeCode));
		CPPUNIT_ASSERT_EQUAL(72, (int)sizeof(BParameterWeb));
		CPPUNIT_ASSERT_EQUAL(264, (int)sizeof(BSmallBuffer));
		CPPUNIT_ASSERT_EQUAL(808, (int)sizeof(BSoundPlayer));
		CPPUNIT_ASSERT_EQUAL(32, (int)sizeof(BTimedEventQueue));
		CPPUNIT_ASSERT_EQUAL(236, (int)sizeof(BTimeSource));
		CPPUNIT_ASSERT_EQUAL(24, (int)sizeof(media_node));
		CPPUNIT_ASSERT_EQUAL(328, (int)sizeof(media_input));
		CPPUNIT_ASSERT_EQUAL(328, (int)sizeof(media_output));
		CPPUNIT_ASSERT_EQUAL(256, (int)sizeof(live_node_info));
		CPPUNIT_ASSERT_EQUAL(372, (int)sizeof(media_request_info));
		CPPUNIT_ASSERT_EQUAL(16, (int)sizeof(media_destination));
		CPPUNIT_ASSERT_EQUAL(16, (int)sizeof(media_source));
		CPPUNIT_ASSERT_EQUAL(200, (int)sizeof(dormant_node_info));
		CPPUNIT_ASSERT_EQUAL(116, (int)sizeof(flavor_info));
		CPPUNIT_ASSERT_EQUAL(320, (int)sizeof(dormant_flavor_info));
		CPPUNIT_ASSERT_EQUAL(192, (int)sizeof(media_format));
#else
		CPPUNIT_ASSERT(true);
#endif
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SizeofTest, getTestSuiteName());
