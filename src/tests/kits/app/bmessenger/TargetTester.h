//------------------------------------------------------------------------------
//	TargetTester.h
//
//------------------------------------------------------------------------------

#ifndef TARGET_TESTER_H
#define TARGET_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TargetTester : public TestCase
{
	public:
		TargetTester() {;}
		TargetTester(std::string name) : TestCase(name) {;}

		void IsTargetLocalTest1();
		void IsTargetLocalTest2();
		void IsTargetLocalTest3();
		void IsTargetLocalTest4();
		void IsTargetLocalTest5();

		void TargetTest1();
		void TargetTest2();
		void TargetTest3();
		void TargetTest4();
		void TargetTest5();

		static Test* Suite();
};

#endif	// TARGET_TESTER_H

