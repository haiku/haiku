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

class BApplication; // forward declaration

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
		void LaunchTestA4();

		void LaunchTestB1();
		void LaunchTestB2();
		void LaunchTestB3();
		void LaunchTestB4();
		void LaunchTestB5();

		void LaunchTestC1();
		void LaunchTestC2();
		void LaunchTestC3();
		void LaunchTestC4();

		void LaunchTestD1();
		void LaunchTestD2();
		void LaunchTestD3();
		void LaunchTestD4();
		void LaunchTestD5();
		void LaunchTestD6();
		void LaunchTestD7();
		void LaunchTestD8();
		void LaunchTestD9();
		void LaunchTestD10();
		void LaunchTestD11();
		void LaunchTestD12();
		void LaunchTestD13();

		void LaunchTestE1();
		void LaunchTestE2();
		void LaunchTestE3();
		void LaunchTestE4();
		void LaunchTestE5();
		void LaunchTestE6();
		void LaunchTestE7();
		void LaunchTestE8();
		void LaunchTestE9();
		void LaunchTestE10();
		void LaunchTestE11();
		void LaunchTestE12();
		void LaunchTestE13();
		void LaunchTestE14();

		void LaunchTestF1();
		void LaunchTestF2();
		void LaunchTestF3();
		void LaunchTestF4();
		void LaunchTestF5();
		void LaunchTestF6();
		void LaunchTestF7();
		void LaunchTestF8();
		void LaunchTestF9();
		void LaunchTestF10();
		void LaunchTestF11();
		void LaunchTestF12();
		void LaunchTestF13();
		void LaunchTestF14();

		static Test* Suite();

private:
	BApplication *fApplication;
};

#endif	// LAUNCH_TESTER_H
