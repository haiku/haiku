//------------------------------------------------------------------------------
//	SetIntervalTester.h
//
//------------------------------------------------------------------------------

#ifndef SET_INTERVAL_TESTER_H
#define SET_INTERVAL_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class SetIntervalTester : public TestCase
{
	public:
		SetIntervalTester() {;}
		SetIntervalTester(std::string name) : TestCase(name) {;}

		void SetInterval1();
		void SetInterval2();
		void SetInterval3();
		void SetInterval4();
		void SetInterval5();
		void SetInterval6();
		void SetInterval7();

		static Test* Suite();
};

#endif	// SET_INTERVAL_TESTER_H
