//------------------------------------------------------------------------------
//	CountTester.h
//
//------------------------------------------------------------------------------

#ifndef COUNT_TESTER_H
#define COUNT_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class CountTester : public TestCase
{
	public:
		CountTester() {;}
		CountTester(std::string name) : TestCase(name) {;}

		void LocalCount1();
		void LocalCount2();
		void LocalCount3();
		void LocalCount4();
		void LocalCount5();
		void LocalCount6();
		void SystemCount1();
		void SystemCount2();
		void SystemCount3();

		static Test* Suite();
};

#endif	// COUNT_TESTER_H

