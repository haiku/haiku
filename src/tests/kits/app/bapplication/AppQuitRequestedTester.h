//------------------------------------------------------------------------------
//	AppQuitRequestedTester.h
//
//------------------------------------------------------------------------------

#ifndef APP_QUIT_REQUESTED_TESTER_H
#define APP_QUIT_REQUESTED_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class AppQuitRequestedTester : public TestCase
{
	public:
		AppQuitRequestedTester() {;}
		AppQuitRequestedTester(std::string name) : TestCase(name) {;}

		void QuitRequestedTest1();

		static Test* Suite();
};

#endif	// APP_QUIT_REQUESTED_TESTER_H

