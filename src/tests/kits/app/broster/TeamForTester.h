//------------------------------------------------------------------------------
//	TeamForTester.h
//
//------------------------------------------------------------------------------

#ifndef TEAM_FOR_TESTER_H
#define TEAM_FOR_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TeamForTester : public TestCase
{
	public:
		TeamForTester() {;}
		TeamForTester(std::string name) : TestCase(name) {;}

		void TeamForTestA1();
		void TeamForTestA2();
		void TeamForTestA3();

		void TeamForTestB1();
		void TeamForTestB2();
		void TeamForTestB3();

		static Test* Suite();
};

#endif	// TEAM_FOR_TESTER_H

