//------------------------------------------------------------------------------
//	HandlerAtTest.h
//
//------------------------------------------------------------------------------

#ifndef HANDLERATTEST_H
#define HANDLERATTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class THandlerAtTest : public TestCase
{
	public:
		THandlerAtTest() {;}
		THandlerAtTest(std::string name) : TestCase(name) {;}

		void HandlerAtTest1();
		void HandlerAtTest2();
		void HandlerAtTest3();
		void HandlerAtTest4();

		static TestSuite* Suite();
};

#endif	//HANDLERATTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

