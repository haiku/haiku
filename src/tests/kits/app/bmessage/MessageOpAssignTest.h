//------------------------------------------------------------------------------
//	MessageOpAssignTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEOPASSIGNTEST_H
#define MESSAGEOPASSIGNTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TMessageOpAssignTest : public TestCase
{
	public:
		TMessageOpAssignTest() {;}
		TMessageOpAssignTest(std::string name) : TestCase(name) {;}

		void MessageOpAssignTest1();

		static TestSuite* Suite();
};

#endif	// MESSAGEOPASSIGNTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

