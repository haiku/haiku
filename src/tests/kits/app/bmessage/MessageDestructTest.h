//------------------------------------------------------------------------------
//	MessageDestructTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEDESTRUCTTEST_H
#define MESSAGEDESTRUCTTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TMessageDestructTest : public TestCase
{
	public:
		TMessageDestructTest() {;}
		TMessageDestructTest(std::string name) : TestCase(name) {;}
		
		void MessageDestructTest1();
		void MessageDestructTest2();

		static TestSuite*	Suite();
};

#endif	// MESSAGEDESTRUCTTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

