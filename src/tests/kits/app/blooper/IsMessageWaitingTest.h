//------------------------------------------------------------------------------
//	IsMessageWaitingTest.h
//
//------------------------------------------------------------------------------

#ifndef ISMESSAGEWAITINGTEST_H
#define ISMESSAGEWAITINGTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TIsMessageWaitingTest : public TestCase
{
	public:
		TIsMessageWaitingTest() {;}
		TIsMessageWaitingTest(std::string name) : TestCase(name) {;}

		void IsMessageWaiting1();
		void IsMessageWaiting2();
		void IsMessageWaiting3();
		void IsMessageWaiting4();
		void IsMessageWaiting5();

		static Test* Suite();
};

#endif	//ISMESSAGEWAITINGTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

