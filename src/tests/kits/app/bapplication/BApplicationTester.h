//------------------------------------------------------------------------------
//	BApplicationTester.h
//
//------------------------------------------------------------------------------

#ifndef B_APPLICATION_TESTER_H
#define B_APPLICATION_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TBApplicationTester : public TestCase
{
	public:
		TBApplicationTester() {;}
		TBApplicationTester(std::string name) : TestCase(name) {;}

		void BApplication1();
		void BApplication2();
		void BApplication3();
		void BApplication4();
		void BApplication5();

		static Test* Suite();
};

#endif	// B_APPLICATION_TESTER_H

