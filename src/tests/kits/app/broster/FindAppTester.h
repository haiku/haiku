//------------------------------------------------------------------------------
//	FindAppTester.h
//
//------------------------------------------------------------------------------

#ifndef FIND_APP_TESTER_H
#define FIND_APP_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class FindAppTester : public TestCase
{
	public:
		FindAppTester() {;}
		FindAppTester(std::string name) : TestCase(name) {;}

		void setUp();
		void tearDown();

		void FindAppTestA1();
		void FindAppTestA2();
		void FindAppTestA3();
		void FindAppTestA4();
		void FindAppTestA5();
		void FindAppTestA6();
		void FindAppTestA7();
		void FindAppTestA8();
		void FindAppTestA9();
		void FindAppTestA10();
		void FindAppTestA11();
		void FindAppTestA12();
		void FindAppTestA13();
		void FindAppTestA14();
		void FindAppTestA15();

		void FindAppTestB1();

		static Test* Suite();

private:
	BApplication *fApplication;
};

#endif	// FIND_APP_TESTER_H
