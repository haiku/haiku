//------------------------------------------------------------------------------
//	UnlockLooperTest.h
//
//------------------------------------------------------------------------------

#ifndef UNLOCKLOOPERTEST_H
#define UNLOCKLOOPERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TUnlockLooperTest : public TestCase
{
	public:
		TUnlockLooperTest() {;}
		TUnlockLooperTest(std::string name) : TestCase(name) {;}

		void UnlockLooper1();
		void UnlockLooper2();
		void UnlockLooper3();

		static Test* Suite();
};

#endif	//UNLOCKLOOPERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

