//------------------------------------------------------------------------------
//	AddHandlerTest.h
//
//------------------------------------------------------------------------------

#ifndef ADDHANDLERTEST_H
#define ADDHANDLERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TAddHandlerTest : public TestCase
{
	public:
		TAddHandlerTest() {;}
		TAddHandlerTest(std::string name) : TestCase(name) {;}

		void AddHandlerTest1();
		void AddHandlerTest2();

		static TestSuite* Suite();
};

#endif	//ADDHANDLERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

