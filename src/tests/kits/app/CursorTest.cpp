/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <Message.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class CursorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CursorTest);
	CPPUNIT_TEST(NullData_Constructor_Fails);
	CPPUNIT_TEST(ValidData_Constructor_Succeeds);
	CPPUNIT_TEST(VariousData_Constructor_Fails);
	CPPUNIT_TEST(NullMessage_Constructor_Fails);
	CPPUNIT_TEST(EmptyMessage_Constructor_Fails);
	CPPUNIT_TEST(Bitmap_Constructor_Succeeds);
	CPPUNIT_TEST(Null_Instantiate_ReturnsNull);
	CPPUNIT_TEST(EmptyMessage_Instantiate_ReturnsNull);
	CPPUNIT_TEST(Null_Archive_ReturnsOk);
	CPPUNIT_TEST(Message_Archive_ReturnsOk);
	CPPUNIT_TEST_SUITE_END();

public:
	void NullData_Constructor_Fails()
	{
		BApplication app("application/x-vnd.cursortest");
		BCursor cur((void*)NULL);
		CPPUNIT_ASSERT(B_OK != cur.InitCheck());
	}

	void ValidData_Constructor_Succeeds()
	{
		BApplication app("application/x-vnd.cursortest");
		char data[68];
		int i;

		data[0] = 16;
		data[1] = 1;
		data[2] = 0;
		data[3] = 0;
		for (i = 4; i < 68; i++)
			data[i] = 1;

		BCursor cur(data);
		CPPUNIT_ASSERT_EQUAL(B_OK, cur.InitCheck());
	}

	void VariousData_Constructor_Fails()
	{
		BApplication app("application/x-vnd.cursortest");
		int x;
		BCursor cur1(&x);
		CPPUNIT_ASSERT(B_OK != cur1.InitCheck());
		char data[68];
		data[0] = 32;
		BCursor cur2(data);
		CPPUNIT_ASSERT(B_OK != cur2.InitCheck());
		data[0] = 16;
		data[1] = 8;
		BCursor cur3(data);
		CPPUNIT_ASSERT(B_OK != cur3.InitCheck());
		data[1] = 1;
		data[2] = 16;
		data[3] = 16;
		BCursor cur4(data);
		CPPUNIT_ASSERT(B_OK != cur4.InitCheck());
	}

	void NullMessage_Constructor_Fails()
	{
		BApplication app("application/x-vnd.cursortest");
		BCursor cur((BMessage*)NULL);
		CPPUNIT_ASSERT(B_OK != cur.InitCheck());
	}

	void EmptyMessage_Constructor_Fails()
	{
		BApplication app("application/x-vnd.cursortest");
		/* The message really should contain a valid archive, but Cursor doesn't
		   support archiving anyway, so until R2, this is a moot point.
		*/
		BMessage msg;
		BCursor cur(&msg);
		CPPUNIT_ASSERT(B_OK != cur.InitCheck());
	}

	void Bitmap_Constructor_Succeeds()
	{
		BApplication app("application/x-vnd.cursortest");

		BBitmap* bitmap;
		BPoint hotspot(0, 0);

		get_mouse_bitmap(&bitmap, &hotspot);
		hotspot.x += 1;
		hotspot.y += 1;
		BCursor cur(bitmap, hotspot);
		CPPUNIT_ASSERT_EQUAL(B_OK, cur.InitCheck());
	}

	void Null_Instantiate_ReturnsNull()
	{
		BApplication app("application/x-vnd.cursortest");
		CPPUNIT_ASSERT(BCursor::Instantiate(NULL) == NULL);
	}

	void EmptyMessage_Instantiate_ReturnsNull()
	{
		BApplication app("application/x-vnd.cursortest");
		/* The message really should contain a valid archive, but Cursor doesn't
		   support archiving anyway, so until R2, this is a moot point.
		*/
		BMessage msg;
		CPPUNIT_ASSERT(BCursor::Instantiate(&msg) == NULL);
	}

	void Null_Archive_ReturnsOk()
	{
		BApplication app("application/x-vnd.cursortest");
		char data[68];
		int i;

		data[0] = 16;
		data[1] = 1;
		data[2] = 0;
		data[3] = 0;
		for (i = 4; i < 68; i++)
			data[i] = 1;

		BCursor cur(data);
		CPPUNIT_ASSERT_EQUAL(B_OK, cur.Archive(NULL));
	}

	void Message_Archive_ReturnsOk()
	{
		BApplication app("application/x-vnd.cursortest");
		char data[68];
		int i;

		data[0] = 16;
		data[1] = 1;
		data[2] = 0;
		data[3] = 0;
		for (i = 4; i < 68; i++)
			data[i] = 1;

		BCursor cur(data);
		BMessage msg;
		CPPUNIT_ASSERT_EQUAL(B_OK, cur.Archive(&msg));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CursorTest, getTestSuiteName());
