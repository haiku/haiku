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
class TValidateInstantiationTest : public TestCase
{
	public:
		TValidateInstantiationTest(std::string name) : TestCase(name) {;}

		void AllParamsInvalid();
		void ClassNameParamInvalid();
		void ArchiveParamInvalid();
		void ClassFieldEmpty();
		void ClassFieldBogus();
		void AllValid();

		static Test* Suite();
};
//------------------------------------------------------------------------------


#endif	//VALIDATEINSTANTIATIONTESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

