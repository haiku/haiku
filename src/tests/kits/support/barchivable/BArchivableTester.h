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

class TBArchivableTestCase : public BTestCase
{
	public:
		TBArchivableTestCase(std::string name = "") : BTestCase(name) {;}

		void TestPerform();
		void InvalidArchiveShallow();
		void ValidArchiveShallow();
		void InvalidArchiveDeep();
		void ValidArchiveDeep();

		static CppUnit::Test* Suite();
};


#endif	//BARCHIVABLETESTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */


