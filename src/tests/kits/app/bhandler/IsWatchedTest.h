//------------------------------------------------------------------------------
//	IsWatchedTest.h
//
//------------------------------------------------------------------------------

#ifndef ISWATCHEDTEST_H
#define ISWATCHEDTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#if defined(TEST_R5)
#include <be/app/Handler.h>
#else
#include <Handler.h>
#endif
// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TIsWatchedTest : public TestCase
{
	public:
		TIsWatchedTest() {;}
		TIsWatchedTest(std::string name) : TestCase(name) {;}

		void IsWatched1();
		void IsWatched2();

		static Test* Suite();

	private:
		BHandler	fHandler;
};

#endif	//ISWATCHEDTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

