//------------------------------------------------------------------------------
//	MessengerComparissonTester.h
//
//------------------------------------------------------------------------------

#ifndef MESSENGER_COMPARISSON_TESTER_H
#define MESSENGER_COMPARISSON_TESTER_H

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

class MessengerComparissonTester : public BThreadedTestCase
{
public:
	MessengerComparissonTester();
	MessengerComparissonTester(std::string name);
	virtual ~MessengerComparissonTester();

	void ComparissonTest1();
	void ComparissonTest2();
	void ComparissonTest3();
	void LessTest1();

	static Test* Suite();

private:
	BHandler *fHandler;
	BLooper *fLooper;
};

#endif	// MESSENGER_COMPARISSON_TESTER_H

