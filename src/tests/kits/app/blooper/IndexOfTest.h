//------------------------------------------------------------------------------
//	IndexOfTest.h
//
//------------------------------------------------------------------------------

#ifndef INDEXOFTEST_H
#define INDEXOFTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TIndexOfTest : public TestCase
{
	public:
		TIndexOfTest() {;}
		TIndexOfTest(std::string name) : TestCase(name) {;}

		void IndexOfTest1();
		void IndexOfTest2();
		void IndexOfTest3();
		void IndexOfTest4();
		void IndexOfTest5();
		void IndexOfTest6();

		static TestSuite* Suite();
};

#endif	//INDEXOFTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

