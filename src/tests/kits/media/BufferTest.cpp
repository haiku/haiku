/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "BufferTest.h"

#include <Application.h>
#include <BufferGroup.h>
#include <Buffer.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


BufferTest::BufferTest()
{
}


BufferTest::~BufferTest()
{
}


void
BufferTest::TestDefault()
{
	// app_server connection (no need to run it)
	BApplication app("application/x-vnd-test"); 
	
	BBufferGroup * group;
	status_t s;
	int32 count;

	group = new BBufferGroup();

	s = group->InitCheck();
	CPPUNIT_ASSERT_EQUAL(B_OK, s);
	
	s = group->CountBuffers(&count);
	CPPUNIT_ASSERT_EQUAL(B_OK, s);
	CPPUNIT_ASSERT_EQUAL(0, count);
}


void
BufferTest::TestRef()
{
	BBufferGroup * group;
	status_t s;
	int32 count;
	BBuffer *buffer;

	group = new BBufferGroup(1234);

	s = group->InitCheck();
	CPPUNIT_ASSERT_EQUAL(B_OK, s);
	
	s = group->CountBuffers(&count);
	CPPUNIT_ASSERT_EQUAL(B_OK, s);
	CPPUNIT_ASSERT_EQUAL(3, count);

	s = group->GetBufferList(1,&buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, s);

	CPPUNIT_ASSERT_EQUAL(1234, buffer->Size());
	CPPUNIT_ASSERT_EQUAL(1234, buffer->SizeAvailable());
	CPPUNIT_ASSERT_EQUAL(0, buffer->SizeUsed());

	media_buffer_id id = buffer->ID();
	BBufferGroup * group2 = new BBufferGroup(1,&id);

	s = group2->InitCheck();
	CPPUNIT_ASSERT_EQUAL(B_OK, s);

	s = group2->CountBuffers(&count);
	CPPUNIT_ASSERT_EQUAL(B_OK, s);
	CPPUNIT_ASSERT_EQUAL(1, count);

	buffer = 0;
	s = group2->GetBufferList(1,&buffer);

	CPPUNIT_ASSERT_EQUAL(1234, buffer->Size());
	CPPUNIT_ASSERT_EQUAL(1234, buffer->SizeAvailable());
	CPPUNIT_ASSERT_EQUAL(0, buffer->SizeUsed());

	delete group;
	delete group2;
}


void
BufferTest::TestSmall()
{
	// FIXME currently not implemented, BSmallBuffer constructor will debugger().
#if 0
	BSmallBuffer * sb = new BSmallBuffer;
	CPPUNIT_ASSERT_EQUAL(0, sb->Size());
	CPPUNIT_ASSERT_EQUAL(0, sb->SizeAvailable());
	CPPUNIT_ASSERT_EQUAL(0, sb->SizeUsed());
	CPPUNIT_ASSERT_EQUAL(0, sb->SmallBufferSizeLimit());

	delete sb;
#endif
}


/*static*/ void
BufferTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("BufferTest");

	suite.addTest(new CppUnit::TestCaller<BufferTest>(
		"BufferTest::TestDefault", &BufferTest::TestDefault));
	suite.addTest(new CppUnit::TestCaller<BufferTest>(
		"BufferTest::TestRef", &BufferTest::TestRef));
	suite.addTest(new CppUnit::TestCaller<BufferTest>(
		"BufferTest::TestSmall", &BufferTest::TestSmall));

	parent.addTest("BufferTest", &suite);
}
