//------------------------------------------------------------------------------
//	LaunchTester.h
//
//------------------------------------------------------------------------------

#ifndef LAUNCH_TESTER_H
#define LAUNCH_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <TestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class LaunchTester : public BTestCase
{
	public:
		LaunchTester() {;}
		LaunchTester(std::string name) : BTestCase(name) {;}

		void setUp();
		void tearDown();

		void LaunchTestA1();
		void LaunchTestA2();
		void LaunchTestA3();

//		void LaunchTestB1();
//		void LaunchTestB2();
//		void LaunchTestB3();
//		void LaunchTestB4();
//		void LaunchTestB5();
//		void LaunchTestB6();
//		void LaunchTestB7();
//		void LaunchTestB8();
//		void LaunchTestB9();
//		void LaunchTestB10();

		static Test* Suite();

private:
	BApplication *fApplication;
};

#endif	// LAUNCH_TESTER_H
