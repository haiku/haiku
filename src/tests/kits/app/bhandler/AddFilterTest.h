//------------------------------------------------------------------------------
//	AddFilterTest.h
//
//------------------------------------------------------------------------------

#ifndef ADDFILTERTEST_H
#define ADDFILTERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#if defined(TEST_R5)
#include <be/app/Handler.h>
#else
#include <Handler.h>
#endif

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TAddFilterTest : public TestCase
{
	public:
		TAddFilterTest() {;}
		TAddFilterTest(std::string name) : TestCase(name) {;}

		void AddFilter1();
		void AddFilter2();
		void AddFilter3();
		void AddFilter4();

		static Test* Suite();
};

#endif	//ADDFILTERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

