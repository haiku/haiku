//------------------------------------------------------------------------------
//	BBitmapTester.h
//
//------------------------------------------------------------------------------

#ifndef B_BITMAP_TESTER_H
#define B_BITMAP_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TBBitmapTester : public TestCase
{
	public:
		TBBitmapTester() {;}
		TBBitmapTester(std::string name) : TestCase(name) {;}

		void BBitmap1();

		static Test* Suite();
};

#endif	// B_BITMAP_TESTER_H

