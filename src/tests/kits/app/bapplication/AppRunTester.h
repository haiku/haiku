//------------------------------------------------------------------------------
//	AppRunTester.h
//
//------------------------------------------------------------------------------

#ifndef APP_RUN_TESTER_H
#define APP_RUN_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class AppRunTester : public TestCase
{
	public:
		AppRunTester() {;}
		AppRunTester(std::string name) : TestCase(name) {;}

		void RunTest1();
		void RunTest2();
		void RunTest3();
		void RunTest4();
		void RunTest5();
		void RunTest6();
		void RunTest7();
		void RunTest8();
		void RunTest9();
		void RunTest10();
		void RunTest11();
		void RunTest12();
		void RunTest13();
		void RunTest14();
		void RunTest15();
		void RunTest16();
		void RunTest17();
		void RunTest18();
		void RunTest19();
		void RunTest20();
		void RunTest21();
		void RunTest22();

		static Test* Suite();
};

#endif	// APP_RUN_TESTER_H

