/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <Looper.h>
#include <Message.h>
#include <Messenger.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


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


class MessageTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MessageTest);
	CPPUNIT_TEST(Constructor_Default_WhatIsZero);
	CPPUNIT_TEST(Constructor_WithWhat_WhatIsCorrect);
	CPPUNIT_TEST(Constructor_Copy_Matches);
	CPPUNIT_TEST(Destructor_WithReply_ReceivedReply);
	CPPUNIT_TEST(Destructor_NoReply_ReceivedNoReply);
	CPPUNIT_TEST(EasyFind_OnEmpty_ReturnsDefaults);
	CPPUNIT_TEST(Assignment_Typical_WhatMatches);
	CPPUNIT_TEST_SUITE_END();

	void _CheckNullConstruction(BMessage& msg)
	{
		CPPUNIT_ASSERT(msg.CountNames(B_ANY_TYPE) == 0);
		CPPUNIT_ASSERT(msg.IsEmpty());
		CPPUNIT_ASSERT(!msg.IsSystem());
		CPPUNIT_ASSERT(!msg.IsReply());
		CPPUNIT_ASSERT(!msg.WasDelivered());
		CPPUNIT_ASSERT(!msg.IsSourceWaiting());
		CPPUNIT_ASSERT(!msg.IsSourceRemote());
	}

public:
	void Constructor_Default_WhatIsZero()
	{
		BMessage msg;
		CPPUNIT_ASSERT(msg.what == 0);
		_CheckNullConstruction(msg);
	}

	/**
		BMessage(uint32 what)
		@case			what initialization constructor
		@params	what	a uint32 message ID code
		@results		BMessage::what == what param
	 */
	void Constructor_WithWhat_WhatIsCorrect()
	{
		BMessage msg(1234);
		CPPUNIT_ASSERT(msg.what == 1234);
		_CheckNullConstruction(msg);
	}

	/**
		BMessage(const BMessage& msg)
		@case			copy of default constructed
		@params	msg		a default constructed BMessage instance
		@results		what == msg.what and ConfirmNullConstruction is good
	 */
	void Constructor_Copy_Matches()
	{
		BMessage msg1(1234);
		CPPUNIT_ASSERT(msg1.what == 1234);
		_CheckNullConstruction(msg1);

		BMessage msg2(msg1);
		CPPUNIT_ASSERT(msg2.what == msg1.what);
		_CheckNullConstruction(msg2);
	}

	void Destructor_WithReply_ReceivedReply()
	{
		BLooper* looper1 = new Looper1("looper1");
		looper1->Run();

		BMessenger msgr(NULL, looper1);
		BMessage reply;
		CPPUNIT_ASSERT_EQUAL(B_OK, msgr.SendMessage('2345', &reply));
		CPPUNIT_ASSERT(reply.what == '3456');

		looper1->Lock();
		looper1->Quit();
	}

	void Destructor_NoReply_ReceivedNoReply()
	{
		BLooper* looper1 = new Looper1("looper1");

		looper1->Run();

		BMessenger msgr(NULL, looper1);
		BMessage reply;
		CPPUNIT_ASSERT_EQUAL(B_OK, msgr.SendMessage('1234', &reply));
		CPPUNIT_ASSERT(reply.what == B_NO_REPLY);

		looper1->Lock();
		looper1->Quit();
	}

	void EasyFind_OnEmpty_ReturnsDefaults()
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

	void Assignment_Typical_WhatMatches()
	{
		BMessage msg1(1234);
		BMessage msg2;
		msg2 = msg1;
		CPPUNIT_ASSERT(msg2.what == msg1.what);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageTest, getTestSuiteName());
