//------------------------------------------------------------------------------
//	TargetTester.h
//
//------------------------------------------------------------------------------

#ifndef TARGET_TESTER_H
#define TARGET_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TargetTester : public TestCase
{
	public:
		TargetTester() {;}
		TargetTester(std::string name) : TestCase(name) {;}

		void Test1();

		static Test* Suite();
};

#endif	// TARGET_TESTER_H

