//------------------------------------------------------------------------------
//	SetFilterListTest.h
//
//------------------------------------------------------------------------------

#ifndef SETFILTERLISTTEST_H
#define SETFILTERLISTTEST_H

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

class TSetFilterListTest : public TestCase
{
	public:
		TSetFilterListTest() {;}
		TSetFilterListTest(std::string name) : TestCase(name) {;}

		void SetFilterList1();
		void SetFilterList2();
		void SetFilterList3();
		void SetFilterList4();
		void SetFilterList5();

		static Test* Suite();
};

#endif	//SETFILTERLISTTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

