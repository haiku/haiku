//------------------------------------------------------------------------------
//	NextHandlerTest.h
//
//------------------------------------------------------------------------------

#ifndef NEXTHANDLERTEST_H
#define NEXTHANDLERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TNextHandlerTest : public TestCase
{
	public:
		TNextHandlerTest() {;}
		TNextHandlerTest(std::string name) : TestCase(name) {;}

		void NextHandler1();
		void NextHandler2();

		static Test* Suite();

	private:
		BHandler	fHandler;
};

#endif	//NEXTHANDLERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

