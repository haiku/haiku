//------------------------------------------------------------------------------
//	SendMessageTester.h
//
//------------------------------------------------------------------------------

#ifndef SEND_MESSAGE_TESTER_H
#define SEND_MESSAGE_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <ThreadedTestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BHandler;
class BLooper;
class SMTester;

class SendMessageTester : public BThreadedTestCase
{
public:
	SendMessageTester();
	SendMessageTester(std::string name);
	virtual ~SendMessageTester();

	void TestUninitialized();
	void TestInitialized(SMTester &tester);

	void SendMessageTest1();

	static Test* Suite();

private:
	BHandler *fHandler;
	BLooper *fLooper;
};

#endif	// SEND_MESSAGE_TESTER_H

