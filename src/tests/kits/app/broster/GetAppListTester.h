//------------------------------------------------------------------------------
//	GetAppListTester.h
//
//------------------------------------------------------------------------------

#ifndef GET_APP_LIST_TESTER_H
#define GET_APP_LIST_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class GetAppListTester : public TestCase
{
	public:
		GetAppListTester() {;}
		GetAppListTester(std::string name) : TestCase(name) {;}

		void GetAppListTestA1();
		void GetAppListTestA2();

		void GetAppListTestB1();
		void GetAppListTestB2();
		void GetAppListTestB3();

		static Test* Suite();
};

#endif	// GET_APP_LIST_TESTER_H

