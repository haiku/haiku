//------------------------------------------------------------------------------
//	PerformTest.h
//
//------------------------------------------------------------------------------

#ifndef PERFORMTEST_H
#define PERFORMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TPerformTest : public TestCase
{
	public:
		TPerformTest() {;}
		TPerformTest(std::string name) : TestCase(name) {;}

		void PerformTest1();

		static TestSuite* Suite();
};

#endif	//PERFORMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

