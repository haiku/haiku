//------------------------------------------------------------------------------
//	BArchivableTester.cpp
//
/**
	BArchivable tests
	@note	InvalidArchiveShallow() and InvalidArchiveDeep() are not tested
			against the original implementation as it does not handle NULL
			parameters gracefully.
 */
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "BArchivableTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	BArchivable::Perform(perform_code d, void* arg)
	@case		Any
	@param d	Not used
	@param arg	Not used
	@results	Returns B_ERROR in all cases.
 */
void TBArchivableTestCase::TestPerform()
{
	BArchivable Archive;
	assert(Archive.Perform(0, NULL) == B_ERROR);
}
//------------------------------------------------------------------------------
/**
	BArchivable::Archive(BMessage* into, bool deep)
	@case		Invalid archive, shallow archiving
	@param into	NULL
	@param deep	false
	@results	Returns B_BAD_VALUE.
 */
void TBArchivableTestCase::InvalidArchiveShallow()
{
	BArchivable Archive;
	assert(Archive.Archive(NULL, false) == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	BArchivable::Archive(BMessage* into, bool deep)
	@case		Valid archive, shallow archiving
	@param into	Valid BMessage pointer
	@param deep false
	@results	Returns B_OK.
				Resultant archive has a string field labeled "class".
				Field "class" contains the string "BArchivable".
 */
void TBArchivableTestCase::ValidArchiveShallow()
{
	BMessage Storage;
	BArchivable Archive;
	assert(Archive.Archive(&Storage, false) == B_OK);
	const char* name;
	assert(Storage.FindString("class", &name) == B_OK);
	assert(strcmp(name, "BArchivable") == 0);
}
//------------------------------------------------------------------------------
/**
	BArchivable::Archive(BMessage* into, bool deep)
	@case		Invalid archive, deep archiving
	@param into	NULL
	@param deep true
	@results	Returns B_BAD_VALUE
 */
void TBArchivableTestCase::InvalidArchiveDeep()
{
	BArchivable Archive;
	assert(Archive.Archive(NULL, true) == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	BArchivable::Archive(BMessage* into, bool deep)
	@case		Valid archive, deep archiving
	@param into	Valid BMessage pointer
	@param deep true
	@results	Returns B_OK.
				Resultant archive has a string field labeled "class".
				Field "class" contains the string "BArchivable".
 */
void TBArchivableTestCase::ValidArchiveDeep()
{
	BMessage Storage;
	BArchivable Archive;
	assert(Archive.Archive(&Storage, true) == B_OK);
	const char* name;
	assert(Storage.FindString("class", &name) == B_OK);
	assert(strcmp(name, "BArchivable") == 0);
}
//------------------------------------------------------------------------------
Test* TBArchivableTestCase::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;
	ADD_TEST(SuiteOfTests, TBArchivableTestCase, TestPerform);
#if !defined(SYSTEM_TEST)
	ADD_TEST(SuiteOfTests, TBArchivableTestCase, InvalidArchiveShallow);
#endif
	ADD_TEST(SuiteOfTests, TBArchivableTestCase, ValidArchiveShallow);
#if !defined(SYSTEM_TEST)
	ADD_TEST(SuiteOfTests, TBArchivableTestCase, InvalidArchiveDeep);
#endif
	ADD_TEST(SuiteOfTests, TBArchivableTestCase, ValidArchiveDeep);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------


/*
 * $Log $
 *
 * $Id  $
 *
 */

