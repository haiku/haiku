//------------------------------------------------------------------------------
//	MessageEasyFindTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <Point.h>
#include <Rect.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageEasyFindTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
void TMessageEasyFindTest::MessageEasyFindTest1()
{
	BRect r(0, 0, -1, -1);
	BPoint p(0, 0);
	BMessage msg;
	CPPUNIT_ASSERT(msg.FindRect("data") == r);
	CPPUNIT_ASSERT(msg.FindPoint("data") == p);
	CPPUNIT_ASSERT(msg.FindString("data") == NULL);
	CPPUNIT_ASSERT(msg.FindInt8("data") == 0);
	CPPUNIT_ASSERT(msg.FindInt16("data") == 0);
	CPPUNIT_ASSERT(msg.FindInt32("data") == 0);
	CPPUNIT_ASSERT(msg.FindInt64("data") == 0);
	CPPUNIT_ASSERT(msg.FindBool("data") == false);
	CPPUNIT_ASSERT(msg.FindFloat("data") == 0);
	CPPUNIT_ASSERT(msg.FindDouble("data") == 0);
}
//------------------------------------------------------------------------------
TestSuite* TMessageEasyFindTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::EasyFinds");

	ADD_TEST4(BMessage, suite, TMessageEasyFindTest, MessageEasyFindTest1);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

