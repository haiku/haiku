//------------------------------------------------------------------------------
//	CountHandlersTest.h
//
//------------------------------------------------------------------------------

#ifndef COUNTHANDLERSTEST_H
#define COUNTHANDLERSTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TCountHandlersTest : public TestCase
{
	public:
		TCountHandlersTest() {;}
		TCountHandlersTest(std::string name) : TestCase(name) {;}

		void CountHandlersTest1();
		void CountHandlersTest2();

		static TestSuite* Suite();
};

#endif	//COUNTHANDLERSTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

