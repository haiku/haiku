//------------------------------------------------------------------------------
//	BCursorTester.h
//
//------------------------------------------------------------------------------

#ifndef B_CURSOR_TESTER_H
#define B_CURSOR_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BCursorTester : public TestCase
{
	public:
		BCursorTester() {;}
		BCursorTester(std::string name) : TestCase(name) {;}

		void BCursor1();
		void BCursor2();
		void BCursor3();
		void BCursor4();
		void BCursor5();
		void BCursor6();
		void Instantiate1();
		void Instantiate2();
		void Archive1();
		void Archive2();

		static Test* Suite();
};

#endif	// B_CURSOR_TESTER_H

