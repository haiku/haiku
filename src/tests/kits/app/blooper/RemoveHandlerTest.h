//------------------------------------------------------------------------------
//	RemoveHandlerTest.h
//
//------------------------------------------------------------------------------

#ifndef REMOVEHANDLERTEST_H
#define REMOVEHANDLERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TRemoveHandlerTest : public TestCase
{
	public:
		TRemoveHandlerTest(std::string name) : TestCase(name) {;}

		void RemoveHandler1();

		static Test* Suite();
};

#endif	//REMOVEHANDLERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

