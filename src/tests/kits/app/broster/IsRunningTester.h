//------------------------------------------------------------------------------
//	IsRunningTester.h
//
//------------------------------------------------------------------------------

#ifndef IS_RUNNING_TESTER_H
#define IS_RUNNING_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class IsRunningTester : public TestCase
{
	public:
		IsRunningTester() {;}
		IsRunningTester(std::string name) : TestCase(name) {;}

		void IsRunningTestA1();
		void IsRunningTestA2();
		void IsRunningTestA3();

		void IsRunningTestB1();
		void IsRunningTestB2();
		void IsRunningTestB3();

		static Test* Suite();
};

#endif	// IS_RUNNING_TESTER_H

