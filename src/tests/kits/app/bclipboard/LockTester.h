//------------------------------------------------------------------------------
//	LockTester.h
//
//------------------------------------------------------------------------------

#ifndef LOCK_TESTER_H
#define LOCK_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class LockTester : public TestCase
{
	public:
		LockTester() {;}
		LockTester(std::string name) : TestCase(name) {;}

		void Lock1();
		void Lock2();
		void IsLocked1();
		void IsLocked2();
		void Unlock1();

		static Test* Suite();
};

#endif	// LOCK_TESTER_H

