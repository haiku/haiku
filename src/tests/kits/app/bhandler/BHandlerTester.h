//------------------------------------------------------------------------------
//	BHandlerTester.h
//
//------------------------------------------------------------------------------

#ifndef BHANDLERTESTER_H
#define BHANDLERTESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TBHandlerTester : public TestCase
{
	public:
		TBHandlerTester() {;}
		TBHandlerTester(std::string name) : TestCase(name) {;}

		void BHandler1();
		void BHandler2();
		void BHandler3();
		void BHandler4();
		void BHandler5();

		void Archive1();
		void Archive2();
		void Archive3();
		void Archive4();

		void Instantiate1();
		void Instantiate2();
		void Instantiate3();

		void SetName1();
		void SetName2();

		void Perform1();

		void FilterList1();

		void UnlockLooper1();
		void UnlockLooper2();
		void UnlockLooper3();

		static Test* Suite();
};

#endif	//BHANDLERTESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

