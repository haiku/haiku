//------------------------------------------------------------------------------
//	LockLooperWithTimeoutTest.h
//
//------------------------------------------------------------------------------

#ifndef LOCKLOOPERWITHTIMEOUTTEST_H
#define LOCKLOOPERWITHTIMEOUTTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TLockLooperWithTimeoutTest : public TestCase
{
	public:
		TLockLooperWithTimeoutTest() {;}
		TLockLooperWithTimeoutTest(std::string name) : TestCase(name) {;}

		void LockLooperWithTimeout1();
		void LockLooperWithTimeout2();
		void LockLooperWithTimeout3();
		void LockLooperWithTimeout4();

		static Test* Suite();
};

#endif	//LOCKLOOPERWITHTIMEOUTTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

