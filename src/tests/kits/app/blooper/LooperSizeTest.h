//------------------------------------------------------------------------------
//	LooperSizeTest.h
//
//------------------------------------------------------------------------------

#ifndef LOOPERSIZETEST_H
#define LOOPERSIZETEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TLooperSizeTest : public TestCase
{
	public:
		TLooperSizeTest() {;}
		TLooperSizeTest(std::string name) : TestCase(name) {;}

		void LooperSizeTest();

		static TestSuite* Suite();
};

#endif	//LOOPERSIZETEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

