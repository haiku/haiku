//------------------------------------------------------------------------------
//	BArchivableTester.h
//
//------------------------------------------------------------------------------

#ifndef BARCHIVABLETESTER_H
#define BARCHIVABLETESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LocalCommon.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class TBArchivableTestCase : public TestCase
{
	public:
		TBArchivableTestCase(std::string name) : TestCase(name) {;}

		void TestPerform();
		void InvalidArchiveShallow();
		void ValidArchiveShallow();
		void InvalidArchiveDeep();
		void ValidArchiveDeep();

		static Test* Suite();
};


#endif	//BARCHIVABLETESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

