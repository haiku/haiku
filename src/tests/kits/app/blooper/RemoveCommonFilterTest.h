//------------------------------------------------------------------------------
//	RemoveCommonFilterTest.h
//
//------------------------------------------------------------------------------

#ifndef REMOVECOMMONFILTERTEST_H
#define REMOVECOMMONFILTERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TRemoveCommonFilterTest : public TestCase
{
	public:
		TRemoveCommonFilterTest() {;}
		TRemoveCommonFilterTest(std::string name) : TestCase(name) {;}

		void RemoveCommonFilterTest1();
		void RemoveCommonFilterTest2();
		void RemoveCommonFilterTest3();
		void RemoveCommonFilterTest4();

		static TestSuite* Suite();
};

#endif	//REMOVECOMMONFILTERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

