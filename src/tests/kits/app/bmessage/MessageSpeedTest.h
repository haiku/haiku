//------------------------------------------------------------------------------
//	MessageSpeedTest.h
//  Written on 04 - 13 - 2005 by Olivier Milla (methedras at online dot fr)
//------------------------------------------------------------------------------

#ifndef MESSAGESPEEDTEST_H
#define MESSAGESPEEDTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TMessageSpeedTest : public TestCase
{
	public:
		TMessageSpeedTest() {;}
		TMessageSpeedTest(std::string name) : TestCase(name) {;}
		
		void MessageSpeedTestCreate5Int32();
		
		void MessageSpeedTestLookup5Int32();
		void MessageSpeedTestLookup50Int32();
		void MessageSpeedTestLookup500Int32();
		
		void MessageSpeedTestRead500Int32();

		void MessageSpeedTestFlatten5Int32(); 
		void MessageSpeedTestFlatten50Int32(); 
		void MessageSpeedTestFlatten500Int32();
		
		void MessageSpeedTestUnflatten5Int32(); 
		void MessageSpeedTestUnflatten50Int32(); 
		void MessageSpeedTestUnflatten500Int32();												
		
		static TestSuite*	Suite();
};

#endif	// MESSAGESPEEDTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

