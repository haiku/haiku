//------------------------------------------------------------------------------
//	MessageInt32ItemTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <posix/string.h>

// System Includes -------------------------------------------------------------
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageInt32ItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

#ifndef USE_TEMPLATES
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest1()
{
	BMessage msg;
	int32 out = 0;
	CPPUNIT_ASSERT(msg.FindInt32("an int32", &out) == B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(out == 0);
	CPPUNIT_ASSERT(msg.FindInt32("an int32") == 0);
	CPPUNIT_ASSERT(!msg.HasInt32("an int32"));
	const void* ptr = &out;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, &ptr, &size) ==
				   B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(ptr == NULL);
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest2()
{
	BMessage msg;
	int32 in = 1234;
	int32 out = 0;
	CPPUNIT_ASSERT(msg.AddInt32("an int32", in) == B_OK);
	CPPUNIT_ASSERT(msg.HasInt32("an int32"));
	CPPUNIT_ASSERT(msg.FindInt32("an int32") == in);
	CPPUNIT_ASSERT(msg.FindInt32("an int32", &out) == B_OK);
	CPPUNIT_ASSERT(out == in);
	int32* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, (const void**)&pout,
				   &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in);
	CPPUNIT_ASSERT(size == sizeof (int32));
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest3()
{
	BMessage msg;
	int32 in = 1234;
	int32 in2 = 7890;
	int32 out = 0;
	CPPUNIT_ASSERT(msg.AddInt32("an int32", in) == B_OK);
	CPPUNIT_ASSERT(msg.ReplaceInt32("an int32", in2) == B_OK);
	CPPUNIT_ASSERT(msg.HasInt32("an int32"));
	CPPUNIT_ASSERT(msg.FindInt32("an int32") == in2);
	CPPUNIT_ASSERT(msg.FindInt32("an int32", &out) == B_OK);
	CPPUNIT_ASSERT(out == in2);
	out = 0;
	int32* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, (const void**)&pout,
				   &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in2);
	CPPUNIT_ASSERT(size == sizeof (int32));
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest4()
{
	BMessage msg;
	int32 out = 0;
	CPPUNIT_ASSERT(msg.FindInt32("an int32", 1, &out) == B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(out == 0);
	CPPUNIT_ASSERT(msg.FindInt32("an int32", 1) == 0);
	CPPUNIT_ASSERT(!msg.HasInt32("an int32", 1));
	const void* ptr = &out;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, 1, &ptr, &size) ==
				   B_NAME_NOT_FOUND);
	CPPUNIT_ASSERT(ptr == NULL);
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest5()
{
	BMessage msg;
	int32 in[] = { 123, 456, 789 };
	int32 out = 0;
	int32* pout;
	ssize_t size;
	
	for (int32 i = 0; i < sizeof (in) / sizeof (int32); ++i)
	{
		CPPUNIT_ASSERT(msg.AddInt32("an int32", in[i]) == B_OK);
	}

	for (int32 i = 0; i < sizeof (in) / sizeof (int32); ++i)
	{
		CPPUNIT_ASSERT(msg.HasInt32("an int32", i));
		CPPUNIT_ASSERT(msg.FindInt32("an int32", i) == in[i]);
		CPPUNIT_ASSERT(msg.FindInt32("an int32", i, &out) == B_OK);
		CPPUNIT_ASSERT(out == in[i]);
		CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, i,
					   (const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(*pout == in[i]);
		CPPUNIT_ASSERT(size == sizeof (int32));
	}
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest6()
{
	BMessage msg;
	int32 in[] = { 123, 456, 789 };
	int32 in2 = 654;
	int32 out = 0;
	const int rIndex = 2;

	for (int32 i = 0; i < sizeof (in) / sizeof (int32); ++i)
	{
		CPPUNIT_ASSERT(msg.AddInt32("an int32", in[i]) == B_OK);
	}

	CPPUNIT_ASSERT(msg.ReplaceInt32("an int32", rIndex, in2) == B_OK);
	CPPUNIT_ASSERT(msg.HasInt32("an int32", rIndex));
	CPPUNIT_ASSERT(msg.FindInt32("an int32", rIndex) == in2);
	CPPUNIT_ASSERT(msg.FindInt32("an int32", rIndex, &out) == B_OK);
	CPPUNIT_ASSERT(out == in2);
	out = 0;
	int32* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, rIndex,
				   (const void**)&pout, &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in2);
	CPPUNIT_ASSERT(size == sizeof (int32));
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest7()
{
	BMessage msg;
	int32 in = 1234;
	int32 out = 0;
	CPPUNIT_ASSERT(msg.AddData("an int32", B_INT32_TYPE, (const void*)&in,
							   sizeof (int32)) == B_OK);
	CPPUNIT_ASSERT(msg.HasInt32("an int32"));
	CPPUNIT_ASSERT(msg.FindInt32("an int32") == in);
	CPPUNIT_ASSERT(msg.FindInt32("an int32", &out) == B_OK);
	CPPUNIT_ASSERT(out == in);
	int32* pout;
	ssize_t size;
	CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, (const void**)&pout,
				   &size) == B_OK);
	CPPUNIT_ASSERT(*pout == in);
	CPPUNIT_ASSERT(size == sizeof (int32));
}
//------------------------------------------------------------------------------
void TMessageInt32ItemTest::MessageInt32ItemTest8()
{
	BMessage msg;
	int32 in[] = { 123, 456, 789 };
	int32 out = 0;
	int32* pout;
	ssize_t size;
	
	for (int32 i = 0; i < sizeof (in) / sizeof (int32); ++i)
	{
		CPPUNIT_ASSERT(msg.AddData("an int32", B_INT32_TYPE,
					   (const void*)&in[i], sizeof (int32)) == B_OK);
	}

	for (int32 i = 0; i < sizeof (in) / sizeof (int32); ++i)
	{
		CPPUNIT_ASSERT(msg.HasInt32("an int32", i));
		CPPUNIT_ASSERT(msg.FindInt32("an int32", i) == in[i]);
		CPPUNIT_ASSERT(msg.FindInt32("an int32", i, &out) == B_OK);
		CPPUNIT_ASSERT(out == in[i]);
		CPPUNIT_ASSERT(msg.FindData("an int32", B_INT32_TYPE, i,
					   (const void**)&pout, &size) == B_OK);
		CPPUNIT_ASSERT(*pout == in[i]);
		CPPUNIT_ASSERT(size == sizeof (int32));
	}
}
//------------------------------------------------------------------------------
TestSuite* TMessageInt32ItemTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::Add/Find/Replace/HasRect()");

	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest1);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest2);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest3);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest4);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest5);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest6);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest7);
	ADD_TEST4(BMessage, suite, TMessageInt32ItemTest, MessageInt32ItemTest8);

	return suite;
}
//------------------------------------------------------------------------------
#endif

/*
 * $Log $
 *
 * $Id  $
 *
 */

