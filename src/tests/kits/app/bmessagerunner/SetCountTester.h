//------------------------------------------------------------------------------
//	SetCountTester.h
//
//------------------------------------------------------------------------------

#ifndef SET_COUNT_TESTER_H
#define SET_COUNT_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class SetCountTester : public TestCase
{
	public:
		SetCountTester() {;}
		SetCountTester(std::string name) : TestCase(name) {;}

		void SetCount1();
		void SetCount2();
		void SetCount3();
		void SetCount4();
		void SetCount5();
		void SetCount6();
		void SetCount7();

		static Test* Suite();
};

#endif	// SET_COUNT_TESTER_H
