//------------------------------------------------------------------------------
//	MessageConstructTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGECONSTRUCTTEST_H
#define MESSAGECONSTRUCTTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BMessage;

class TMessageConstructTest : public TestCase
{
	public:
		TMessageConstructTest() {;}
		TMessageConstructTest(std::string name) : TestCase(name) {;}

		void MessageConstructTest1();
		void MessageConstructTest2();
		void MessageConstructTest3();

		static TestSuite*	Suite();

	private:
		void ConfirmNullConstruction(BMessage& msg);
};

#endif	//MESSAGECONSTRUCTTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

