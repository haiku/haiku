//------------------------------------------------------------------------------
//	MessageEasyFindTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEEASYFINDTEST_H
#define MESSAGEEASYFINDTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TMessageEasyFindTest : public TestCase
{
	public:
		TMessageEasyFindTest() {;}
		TMessageEasyFindTest(std::string name) : TestCase(name) {;}

		void MessageEasyFindTest1();

		static TestSuite* Suite();
};

#endif	// MESSAGEEASYFINDTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

