//------------------------------------------------------------------------------
//	BMessengerTester.h
//
//------------------------------------------------------------------------------

#ifndef B_MESSENGER_TESTER_H
#define B_MESSENGER_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TBMessengerTester : public TestCase
{
	public:
		TBMessengerTester() {;}
		TBMessengerTester(std::string name) : TestCase(name) {;}

		void BMessenger1();
		void BMessenger2();
		void BMessenger3();
		void BMessenger4();
		void BMessenger5();
		void BMessenger6();
		void BMessenger7();
		void BMessenger8();
		void BMessenger9();
		void BMessenger10();

		void BMessengerD1();
		void BMessengerD2();
		void BMessengerD3();
		void BMessengerD4();
		void BMessengerD5();
		void BMessengerD6();
		void BMessengerD7();
		void BMessengerD8();
		void BMessengerD9();

		static Test* Suite();
};

#endif	// B_MESSENGER_TESTER_H

