//------------------------------------------------------------------------------
//	LooperTest.h
//
//------------------------------------------------------------------------------

#ifndef LOOPERTEST_H
#define LOOPERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TLooperTest : public TestCase
{
	public:
		TLooperTest() {;}
		TLooperTest(std::string name) : TestCase(name) {;}

		void LooperTest1();
		void LooperTest2();

		static Test* Suite();

	private:
		BHandler	fHandler;
};

#endif	//LOOPERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

