//------------------------------------------------------------------------------
//	AddCommonFilterTest.h
//
//------------------------------------------------------------------------------

#ifndef ADDCOMMONFILTERTEST_H
#define ADDCOMMONFILTERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TAddCommonFilterTest : public TestCase
{
	public:
		TAddCommonFilterTest() {;}
		TAddCommonFilterTest(std::string name) : TestCase(name) {;}

		void AddCommonFilterTest1();
		void AddCommonFilterTest2();
		void AddCommonFilterTest3();
		void AddCommonFilterTest4();

		static TestSuite* Suite();
};

#endif	//ADDCOMMONFILTERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

