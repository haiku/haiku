//------------------------------------------------------------------------------
//	MessageInt32ItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEINT32ITEMTEST_H
#define MESSAGEINT32ITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TMessageInt32ItemTest : public TestCase
{
	public:
		TMessageInt32ItemTest() {;}
		TMessageInt32ItemTest(std::string name) : TestCase(name) {;}

		void MessageInt32ItemTest1();
		void MessageInt32ItemTest2();
		void MessageInt32ItemTest3();
		void MessageInt32ItemTest4();
		void MessageInt32ItemTest5();
		void MessageInt32ItemTest6();
		void MessageInt32ItemTest7();
		void MessageInt32ItemTest8();

		static TestSuite* Suite();
};

#endif	// MESSAGEINT32ITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

