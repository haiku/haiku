//------------------------------------------------------------------------------
//	RemoveFilterTest.h
//
//------------------------------------------------------------------------------

#ifndef REMOVEFILTERTEST_H
#define REMOVEFILTERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TRemoveFilterTest : public TestCase
{
	public:
		TRemoveFilterTest() {;}
		TRemoveFilterTest(std::string name) : TestCase(name) {;}

		void RemoveFilter1();
		void RemoveFilter2();
		void RemoveFilter3();
		void RemoveFilter4();
		void RemoveFilter5();
		void RemoveFilter6();
		void RemoveFilter7();

		static Test* Suite();
};

#endif	//REMOVEFILTERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

