//------------------------------------------------------------------------------
//	MessageConstructTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"
#include "MessageConstructTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	BMessage()
	@case		Default construction
	@params		none
	@results	BMessage::what == 0
 */
void TMessageConstructTest::MessageConstructTest1()
{
	BMessage msg;
	CPPUNIT_ASSERT(msg.what == 0);
	ConfirmNullConstruction(msg);
}
//------------------------------------------------------------------------------
/**
	BMessage(uint32 what)
	@case			what initialization constructor
	@params	what	a uint32 message ID code
	@results		BMessage::what == what param
 */
void TMessageConstructTest::MessageConstructTest2()
{
	BMessage msg(1234);
	CPPUNIT_ASSERT(msg.what == 1234);
	ConfirmNullConstruction(msg);
}
//------------------------------------------------------------------------------
/**
	BMessage(const BMessage& msg)
	@case			copy of default constructed
	@params	msg		a default constructed BMessage instance
	@results		what == msg.what and ConfirmNullConstruction is good
 */
void TMessageConstructTest::MessageConstructTest3()
{
	BMessage msg1(1234);
	CPPUNIT_ASSERT(msg1.what == 1234);
	ConfirmNullConstruction(msg1);

	BMessage msg2(msg1);
	CPPUNIT_ASSERT(msg2.what == msg1.what);
	ConfirmNullConstruction(msg2);
}
//------------------------------------------------------------------------------
TestSuite* TMessageConstructTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::BMessage()");

	ADD_TEST4(BMessage, suite, TMessageConstructTest, MessageConstructTest1);
	ADD_TEST4(BMessage, suite, TMessageConstructTest, MessageConstructTest2);

	return suite;
}
//------------------------------------------------------------------------------
void TMessageConstructTest::ConfirmNullConstruction(BMessage& msg)
{
	CPPUNIT_ASSERT(msg.CountNames(B_ANY_TYPE) == 0);
	CPPUNIT_ASSERT(msg.IsEmpty());
	CPPUNIT_ASSERT(!msg.IsSystem());
	CPPUNIT_ASSERT(!msg.IsReply());
	CPPUNIT_ASSERT(!msg.WasDelivered());
	CPPUNIT_ASSERT(!msg.IsSourceWaiting());
	CPPUNIT_ASSERT(!msg.IsSourceRemote());
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

