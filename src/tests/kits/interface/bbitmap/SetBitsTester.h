//------------------------------------------------------------------------------
//	SetBitsTester.h
//
//------------------------------------------------------------------------------

#ifndef SET_BITS_TESTER_H
#define SET_BITS_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class SetBitsTester : public TestCase
{
	public:
		SetBitsTester() {;}
		SetBitsTester(std::string name) : TestCase(name) {;}

		void SetBits1();
		void SetBits2();
		void SetBits3();
		void SetBits4();

// new API
#ifndef TEST_R5
		void ImportBitsA1();
		void ImportBitsA2();
		void ImportBitsA3();
#endif

		static Test* Suite();
};

#endif	// SET_BITS_TESTER_H

