//------------------------------------------------------------------------------
//	FindAppTester.h
//
//------------------------------------------------------------------------------

#ifndef FIND_APP_TESTER_H
#define FIND_APP_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <TestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

class BApplication; // forward declaration

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class FindAppTester : public BTestCase
{
	public:
		FindAppTester() {;}
		FindAppTester(std::string name) : BTestCase(name) {;}

		void setUp();
		void tearDown();

		void FindAppTestA1();
		void FindAppTestA2();
		void FindAppTestA3();

		void FindAppTestB1();
		void FindAppTestB2();
		void FindAppTestB3();
		void FindAppTestB4();
		void FindAppTestB5();
		void FindAppTestB6();
		void FindAppTestB7();
		void FindAppTestB8();
		void FindAppTestB9();
		void FindAppTestB10();
		void FindAppTestB11();
		void FindAppTestB12();

		static Test* Suite();

private:
	BApplication *fApplication;
};

#endif	// FIND_APP_TESTER_H
