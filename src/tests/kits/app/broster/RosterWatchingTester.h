//------------------------------------------------------------------------------
//	RosterWatchingTester.h
//
//------------------------------------------------------------------------------

#ifndef ROSTER_WATCHING_TESTER_H
#define ROSTER_WATCHING_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <TestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

class BApplication; // forward declaration

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class RosterWatchingTester : public BTestCase
{
	public:
		RosterWatchingTester() {;}
		RosterWatchingTester(std::string name) : BTestCase(name) {;}

		void setUp();
		void tearDown();

		void WatchingTest1();
		void WatchingTest2();
		void WatchingTest3();

		static Test* Suite();

private:
	BApplication *fApplication;
};

#endif	// ROSTER_WATCHING_TESTER_H
