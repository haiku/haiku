//------------------------------------------------------------------------------
//	BClipboardTester.h
//
//------------------------------------------------------------------------------

#ifndef B_CLIPBOARD_TESTER_H
#define B_CLIPBOARD_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BClipboardTester : public TestCase
{
	public:
		BClipboardTester() {;}
		BClipboardTester(std::string name) : TestCase(name) {;}

		void BClipboard1();
		void BClipboard2();

		static Test* Suite();
};

#endif	// B_CLIPBOARD_TESTER_H

