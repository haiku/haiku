//------------------------------------------------------------------------------
//	GetAppInfoTester.h
//
//------------------------------------------------------------------------------

#ifndef GET_APP_INFO_TESTER_H
#define GET_APP_INFO_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class GetAppInfoTester : public TestCase
{
	public:
		GetAppInfoTester() {;}
		GetAppInfoTester(std::string name) : TestCase(name) {;}

		void GetAppInfoTestA1();
		void GetAppInfoTestA2();
		void GetAppInfoTestA3();

		void GetAppInfoTestB1();
		void GetAppInfoTestB2();
		void GetAppInfoTestB3();

		void GetRunningAppInfoTest1();
		void GetRunningAppInfoTest2();
		void GetRunningAppInfoTest3();

		static Test* Suite();
};

#endif	// GET_APP_INFO_TESTER_H

