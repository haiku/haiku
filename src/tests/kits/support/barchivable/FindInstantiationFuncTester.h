//------------------------------------------------------------------------------
//	FindInstatiationFuncTester.h
//
//------------------------------------------------------------------------------

#ifndef FINDINSTATIATIONFUNCTESTER_H
#define FINDINSTATIATIONFUNCTESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LocalCommon.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TFindInstantiationFuncTester : public BTestCase
{
	public:
		TFindInstantiationFuncTester(std::string name = "") : BTestCase(name) {;}

		void Case1();
		void Case2();
		void Case3();
		void Case4();
		void Case5();
		void Case6();
		void Case7();
		void Case8();
		void Case9();
		void Case10();

		// BMessage using versions
		void Case1M();
		void Case2M();
		void Case3M();
		void Case4M();
		void Case5M();
		void Case6M();
		void Case7M();
		void Case8M();
		void Case9M();
		void Case10M();

		static CppUnit::Test* Suite();
};

#endif	//FINDINSTATIATIONFUNCTESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */


