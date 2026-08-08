/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Buffer.h>
#include <BufferGroup.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class BufferTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(BufferTest);
	CPPUNIT_TEST(DefaultBufferGroup_InitCheckAndCount_ReturnsCorrectValues);
	CPPUNIT_TEST(DefaultBufferGroup_Size_ReturnsThreeBuffersAndCorrectSize);
	CPPUNIT_TEST(TwoBufferGroups_ReferencingBufferFromOtherGroup_ReturnsCorrectSize);
	CPPUNIT_TEST_SUITE_END();

public:
	void DefaultBufferGroup_InitCheckAndCount_ReturnsCorrectValues()
	{
		// app_server connection (no need to run it)
		BApplication app("application/x-vnd-test");

		BBufferGroup* group;
		status_t s;
		int32 count;

		group = new BBufferGroup();

		s = group->InitCheck();
		CPPUNIT_ASSERT_EQUAL(B_OK, s);

		s = group->CountBuffers(&count);
		CPPUNIT_ASSERT_EQUAL(B_OK, s);
		CPPUNIT_ASSERT_EQUAL(0, count);

		delete group;
	}
	
	void DefaultBufferGroup_Size_ReturnsThreeBuffersAndCorrectSize()
	{
		BBufferGroup* group;
		status_t s;
		int32 count;
		BBuffer* buffer;

		group = new BBufferGroup(1234);

		s = group->InitCheck();
		CPPUNIT_ASSERT_EQUAL(B_OK, s);

		s = group->CountBuffers(&count);
		CPPUNIT_ASSERT_EQUAL(B_OK, s);
		CPPUNIT_ASSERT_EQUAL(3, count);

		s = group->GetBufferList(1, &buffer);
		CPPUNIT_ASSERT_EQUAL(B_OK, s);

		CPPUNIT_ASSERT_EQUAL(1234, static_cast<int>(buffer->Size()));
		CPPUNIT_ASSERT_EQUAL(1234, static_cast<int>(buffer->SizeAvailable()));
		CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(buffer->SizeUsed()));

		delete group;
	}

	void TwoBufferGroups_ReferencingBufferFromOtherGroup_ReturnsCorrectSize() {

		BBufferGroup* group;
		status_t s;
		int32 count;
		BBuffer* buffer;

		group = new BBufferGroup(1234);

		s = group->InitCheck();
		CPPUNIT_ASSERT_EQUAL(B_OK, s);

		s = group->GetBufferList(1, &buffer);
		CPPUNIT_ASSERT_EQUAL(B_OK, s);

		media_buffer_id id = buffer->ID();
		BBufferGroup* group2 = new BBufferGroup(1, &id);

		s = group2->InitCheck();
		CPPUNIT_ASSERT_EQUAL(B_OK, s);

		s = group2->CountBuffers(&count);
		CPPUNIT_ASSERT_EQUAL(B_OK, s);
		CPPUNIT_ASSERT_EQUAL(1, count);

		buffer = NULL;
		s = group2->GetBufferList(1, &buffer);

		CPPUNIT_ASSERT_EQUAL(1234, static_cast<int>(buffer->Size()));
		CPPUNIT_ASSERT_EQUAL(1234, static_cast<int>(buffer->SizeAvailable()));
		CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(buffer->SizeUsed()));

		delete group;
		delete group2;
	}

	void
	SmallBuffer_Size_ReturnsCorrectValues()
	{
		BSmallBuffer * sb = new BSmallBuffer;
		CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(sb->Size()));
		CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(sb->SizeAvailable()));
		CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(sb->SizeUsed()));
		CPPUNIT_ASSERT_EQUAL(0, static_cast<int>(sb->SmallBufferSizeLimit()));

		delete sb;
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(BufferTest, getTestSuiteName());
