/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "SizeofTest.h"

#include <MediaKit.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


SizeofTest::SizeofTest()
{
}


SizeofTest::~SizeofTest()
{
}


void
SizeofTest::TestSize()
{
	CPPUNIT_ASSERT_EQUAL(264, sizeof(BBuffer));
	CPPUNIT_ASSERT_EQUAL(240, sizeof(BBufferConsumer));
	CPPUNIT_ASSERT_EQUAL(56, sizeof(BBufferGroup));
	CPPUNIT_ASSERT_EQUAL(244, sizeof(BBufferProducer));
	CPPUNIT_ASSERT_EQUAL(140, sizeof(BContinuousParameter));
	CPPUNIT_ASSERT_EQUAL(240, sizeof(BControllable));
	CPPUNIT_ASSERT_EQUAL(124, sizeof(BDiscreteParameter));
	CPPUNIT_ASSERT_EQUAL(236, sizeof(BFileInterface));
	CPPUNIT_ASSERT_EQUAL(40, sizeof(BMediaAddOn));
	CPPUNIT_ASSERT_EQUAL(340, sizeof(BMediaEventLooper));
	CPPUNIT_ASSERT_EQUAL(560, sizeof(BMediaFile));
	CPPUNIT_ASSERT_EQUAL(72, sizeof(BMediaFiles));
	CPPUNIT_ASSERT_EQUAL(128, sizeof(BMediaFormats));
	CPPUNIT_ASSERT_EQUAL(164, sizeof(BMediaNode));
	CPPUNIT_ASSERT_EQUAL(440, sizeof(BMediaRoster));
	CPPUNIT_ASSERT_EQUAL(68, sizeof(BMediaTheme));
	CPPUNIT_ASSERT_EQUAL(760, sizeof(BMediaTrack));
	CPPUNIT_ASSERT_EQUAL(116, sizeof(BNullParameter));
	CPPUNIT_ASSERT_EQUAL(84, sizeof(BParameter));
	CPPUNIT_ASSERT_EQUAL(52, sizeof(BParameterGroup));
	CPPUNIT_ASSERT_EQUAL(124, sizeof(BSound));
	CPPUNIT_ASSERT_EQUAL(164, sizeof(BTimeCode));
	CPPUNIT_ASSERT_EQUAL(72, sizeof(BParameterWeb));
	CPPUNIT_ASSERT_EQUAL(264, sizeof(BSmallBuffer));
	CPPUNIT_ASSERT_EQUAL(808, sizeof(BSoundPlayer));
	CPPUNIT_ASSERT_EQUAL(32, sizeof(BTimedEventQueue));
	CPPUNIT_ASSERT_EQUAL(236, sizeof(BTimeSource));
	CPPUNIT_ASSERT_EQUAL(24, sizeof(media_node));
	CPPUNIT_ASSERT_EQUAL(328, sizeof(media_input));
	CPPUNIT_ASSERT_EQUAL(328, sizeof(media_output));
	CPPUNIT_ASSERT_EQUAL(256, sizeof(live_node_info));
	CPPUNIT_ASSERT_EQUAL(372, sizeof(media_request_info));
	CPPUNIT_ASSERT_EQUAL(16, sizeof(media_destination));
	CPPUNIT_ASSERT_EQUAL(16, sizeof(media_source));
	CPPUNIT_ASSERT_EQUAL(200, sizeof(dormant_node_info));
	CPPUNIT_ASSERT_EQUAL(116, sizeof(flavor_info));
	CPPUNIT_ASSERT_EQUAL(320, sizeof(dormant_flavor_info));
	CPPUNIT_ASSERT_EQUAL(192, sizeof(media_format));

//	printf("BMediaBufferDecoder sizeof = %ld\n",sizeof(BMediaBufferDecoder));
//	printf("MediaBufferEncoder sizeof = %ld\n",sizeof(MediaBufferEncoder));
//	printf("BMediaDecoder sizeof = %ld\n",sizeof(BMediaDecoder));
//	printf("BMediaEncoder  sizeof = %ld\n",sizeof(BMediaEncoder));
}


/*static*/ void
SizeofTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("AreaTest");

	suite.addTest(new CppUnit::TestCaller<SizeofTest>(
		"SizeofTest::TestSize", &SizeofTest::TestSize));

	parent.addTest("SizeofTest", &suite);
}
