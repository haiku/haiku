//------------------------------------------------------------------------------
//	MessageOpAssignTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageOpAssignTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
void TMessageOpAssignTest::MessageOpAssignTest1()
{
	BMessage msg1(1234);
	BMessage msg2;
	msg2 = msg1;
	CPPUNIT_ASSERT(msg2.what == msg1.what);
}
//------------------------------------------------------------------------------
TestSuite* TMessageOpAssignTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::operator=(const BMessage&)");
	
	ADD_TEST4(BMessage, suite, TMessageOpAssignTest, MessageOpAssignTest1);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

