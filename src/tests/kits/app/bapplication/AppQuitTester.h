//------------------------------------------------------------------------------
//	AppQuitTester.h
//
//------------------------------------------------------------------------------

#ifndef APP_QUIT_TESTER_H
#define APP_QUIT_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class AppQuitTester : public TestCase
{
	public:
		AppQuitTester() {;}
		AppQuitTester(std::string name) : TestCase(name) {;}

		void QuitTest1();
		void QuitTest2();
		void QuitTest3();
		void QuitTest4();

		static Test* Suite();
};

#endif	// APP_QUIT_TESTER_H

