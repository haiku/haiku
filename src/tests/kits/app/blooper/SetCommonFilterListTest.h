//------------------------------------------------------------------------------
//	SetCommonFilterTest.h
//
//------------------------------------------------------------------------------

#ifndef SETCOMMONFILTERTEST_H
#define SETCOMMONFILTERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TSetCommonFilterListTest : public TestCase
{
	public:
		TSetCommonFilterListTest() {;}
		TSetCommonFilterListTest(std::string name) : TestCase(name) {;}

		void SetCommonFilterListTest1();
		void SetCommonFilterListTest2();
		void SetCommonFilterListTest3();
		void SetCommonFilterListTest4();

		static TestSuite* Suite();
};

#endif	//SETCOMMONFILTERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

