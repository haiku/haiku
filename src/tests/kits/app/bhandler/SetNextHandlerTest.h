//------------------------------------------------------------------------------
//	SetNextHandlerTest.h
//
//------------------------------------------------------------------------------

#ifndef SETNEXTHANDLERTEST_H
#define SETNEXTHANDLERTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TSetNextHandlerTest : public TestCase
{
	public:
		TSetNextHandlerTest() {;}
		TSetNextHandlerTest(std::string name) : TestCase(name) {;}

		void SetNextHandler0();
		void SetNextHandler1();
		void SetNextHandler2();
		void SetNextHandler3();
		void SetNextHandler4();
		void SetNextHandler5();
		void SetNextHandler6();
		void SetNextHandler7();
		void SetNextHandler8();
		void SetNextHandler9();
		void SetNextHandler10();
		void SetNextHandler11();

		static Test* Suite();

	private:
};

#endif	//SETNEXTHANDLERTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

