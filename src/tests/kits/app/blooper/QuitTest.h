//------------------------------------------------------------------------------
//	QuitTest.h
//
//------------------------------------------------------------------------------

#ifndef QUITTEST_H
#define QUITTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TQuitTest : public TestCase
{
	public:
		TQuitTest() {;}
		TQuitTest(std::string name) : TestCase(name) {;}

		void QuitTest1();
		void QuitTest2();

		static TestSuite* Suite();
};

#endif	//QUITTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

