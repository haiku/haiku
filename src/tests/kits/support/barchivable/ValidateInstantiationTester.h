//------------------------------------------------------------------------------
//	ValidateInstantiationTester.h
//
//------------------------------------------------------------------------------

#ifndef VALIDATEINSTANTIATIONTESTER_H
#define VALIDATEINSTANTIATIONTESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include "LocalCommon.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
class TValidateInstantiationTest : public BTestCase
{
	public:
		TValidateInstantiationTest(std::string name = "") : BTestCase(name) {;}

		void AllParamsInvalid();
		void ClassNameParamInvalid();
		void ArchiveParamInvalid();
		void ClassFieldEmpty();
		void ClassFieldBogus();
		void AllValid();

		static CppUnit::Test* Suite();
};
//------------------------------------------------------------------------------


#endif	//VALIDATEINSTANTIATIONTESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */


