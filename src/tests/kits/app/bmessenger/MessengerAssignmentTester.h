//------------------------------------------------------------------------------
//	MessengerAssignmentTester.h
//
//------------------------------------------------------------------------------

#ifndef MESSENGER_ASSIGNMENT_TESTER_H
#define MESSENGER_ASSIGNMENT_TESTER_H

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

class MessengerAssignmentTester : public BThreadedTestCase
{
public:
	MessengerAssignmentTester();
	MessengerAssignmentTester(std::string name);
	virtual ~MessengerAssignmentTester();

	void AssignmentTest1();
	void AssignmentTest2();
	void AssignmentTest3();

	static Test* Suite();

private:
	BHandler *fHandler;
	BLooper *fLooper;
};

#endif	// MESSENGER_ASSIGNMENT_TESTER_H

