//------------------------------------------------------------------------------
//	GetInfoTester.h
//
//------------------------------------------------------------------------------

#ifndef GET_INFO_TESTER_H
#define GET_INFO_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class GetInfoTester : public TestCase
{
	public:
		GetInfoTester() {;}
		GetInfoTester(std::string name) : TestCase(name) {;}

		void GetInfo1();
		void GetInfo2();

		static Test* Suite();
};

#endif	// GET_INFO_TESTER_H
