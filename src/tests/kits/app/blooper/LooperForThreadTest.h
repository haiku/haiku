//------------------------------------------------------------------------------
//	LooperForThreadTest.h
//
//------------------------------------------------------------------------------

#ifndef LOOPERFORTHREADTEST_H
#define LOOPERFORTHREADTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TLooperForThreadTest : public TestCase
{
	public:
		TLooperForThreadTest() {;}
		TLooperForThreadTest(std::string name) : TestCase(name) {;}

		void LooperForThreadTest1();
		void LooperForThreadTest2();

		static TestSuite* Suite();
};

#endif	//LOOPERFORTHREADTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

