//------------------------------------------------------------------------------
//	BMessageRunnerTester.h
//
//------------------------------------------------------------------------------

#ifndef B_MESSAGE_RUNNER_TESTER_H
#define B_MESSAGE_RUNNER_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TBMessageRunnerTester : public TestCase
{
	public:
		TBMessageRunnerTester() {;}
		TBMessageRunnerTester(std::string name) : TestCase(name) {;}

		void BMessageRunnerA1();
		void BMessageRunnerA2();
		void BMessageRunnerA3();
		void BMessageRunnerA4();
		void BMessageRunnerA5();
		void BMessageRunnerA6();
		void BMessageRunnerA7();
		void BMessageRunnerA8();

		void BMessageRunnerB1();
		void BMessageRunnerB2();
		void BMessageRunnerB3();
		void BMessageRunnerB4();
		void BMessageRunnerB5();
		void BMessageRunnerB6();
		void BMessageRunnerB7();
		void BMessageRunnerB8();
		void BMessageRunnerB9();

		static Test* Suite();
};

#endif	// B_MESSAGE_RUNNER_TESTER_H
