//------------------------------------------------------------------------------
//	BroadcastTester.h
//
//------------------------------------------------------------------------------

#ifndef BROADCAST_TESTER_H
#define BROADCAST_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <TestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

class BApplication; // forward declaration

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BroadcastTester : public BTestCase
{
	public:
		BroadcastTester() {;}
		BroadcastTester(std::string name) : BTestCase(name) {;}

		void setUp();
		void tearDown();

		void BroadcastTestA1();
		void BroadcastTestA2();

		void BroadcastTestB1();
		void BroadcastTestB2();
		void BroadcastTestB3();

		static Test* Suite();

private:
	BApplication *fApplication;
};

#endif	// BROADCAST_TESTER_H
