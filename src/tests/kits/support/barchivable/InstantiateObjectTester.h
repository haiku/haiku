//------------------------------------------------------------------------------
//	InstantiateObjectTester.h
//
//------------------------------------------------------------------------------

#ifndef INSTANTIATEOBJECTTESTER_H
#define INSTANTIATEOBJECTTESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LocalCommon.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
class TInstantiateObjectTester : public TestCase
{
	public:
		TInstantiateObjectTester(std::string name);

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
		void Case11();
		void Case12();
		void Case13();
		void Case14();

		static Test* Suite();

	private:
		void		LoadAddon();
		void		UnloadAddon();
		std::string	GetLocalSignature();

		image_id	fAddonId;
};
//------------------------------------------------------------------------------

#endif	//INSTANTIATEOBJECTTESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

