//------------------------------------------------------------------------------
//	MessageDestructTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageDestructTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	~BMessage
	@case			no reply to sent message
	@results		B_NO_REPLY reply is received
 */
class Looper1 : public BLooper
{
	public:
		Looper1(const char* name) : BLooper(name) {;}
		
		void MessageReceived(BMessage* msg)
		{
			switch (msg->what)
			{
				case '1234':
					break;
				case '2345':
					msg->SendReply('3456');
					break;
				default:
					BLooper::MessageReceived(msg);
			}
		}
};

void TMessageDestructTest::MessageDestructTest1()
{
	BLooper* looper1 = new Looper1("looper1");

	looper1->Run();

	BMessenger msgr(NULL, looper1);
	BMessage reply;
	CPPUNIT_ASSERT(msgr.SendMessage('1234', &reply) == B_OK);
	CPPUNIT_ASSERT(reply.what == B_NO_REPLY);

	looper1->Lock();
	looper1->Quit();
}
//------------------------------------------------------------------------------
/**
	~BMessage
	@case			Reply is sent to message
	@result			No B_NO_REPLY reply is sent
 */
void TMessageDestructTest::MessageDestructTest2()
{
	BLooper* looper1 = new Looper1("looper1");
	looper1->Run();
	
	BMessenger msgr(NULL, looper1);
	BMessage reply;
	CPPUNIT_ASSERT(msgr.SendMessage('2345', &reply) == B_OK);
	CPPUNIT_ASSERT(reply.what == '3456');

	looper1->Lock();
	looper1->Quit();
}
//------------------------------------------------------------------------------
TestSuite* TMessageDestructTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::~BMessage()");

	ADD_TEST4(BMessage, suite, TMessageDestructTest, MessageDestructTest1);
	ADD_TEST4(BMessage, suite, TMessageDestructTest, MessageDestructTest2);

	return suite;
}
//------------------------------------------------------------------------------


/*
 * $Log $
 *
 * $Id  $
 *
 */

