//------------------------------------------------------------------------------
//	ValidateInstantiationTester.cpp
//
/**
	Testing for validate_instantiation(BMessage* archive, const char* className)
	@note	The AllParamsInvalid() and ArchiveInvalid() test are not to be run
			against the original implementation, as it does not validate the
			archive parameter with a resulting segment violation.
 */
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <errno.h>

// System Includes -------------------------------------------------------------
#include <Message.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "ValidateInstantiationTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
const char* gClassName		= "FooBar";
const char* gBogusClassName	= "BarFoo";

//------------------------------------------------------------------------------
/**
	validate_instantiation(BMessage* archive, const char* className)
	@case				All parameters invalid (i.e., NULL)
	@param archive		NULL
	@param className	NULL
	@results			Returns false.
						errno is set to B_BAD_VALUE.
 */
void TValidateInstantiationTest::AllParamsInvalid()
{
	errno = B_OK;
	CPPUNIT_ASSERT(!validate_instantiation(NULL, NULL));
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	validate_instantiation(BMessage* archive, const char* className)
	@case				Valid archive, invalid className (i.e., NULL)
	@param archive		Valid BMessage pointer
	@param className	NULL
	@results			Returns false.
						errno is set to B_MISMATCHED_VALUES.
 */
void TValidateInstantiationTest::ClassNameParamInvalid()
{
	errno = B_OK;
	BMessage Archive;
	CPPUNIT_ASSERT(!validate_instantiation(&Archive, NULL));
	CPPUNIT_ASSERT(errno == B_MISMATCHED_VALUES);
}
//------------------------------------------------------------------------------
/**
	validate_instantiation(BMessage* archive, const char* className)
	@case				Invalid archive (i.e., NULL), valid className
	@param archive		NULL
	@param className	A valid C-string
	@results			Returns false.
						errno is set to B_BAD_VALUE.
	@note				Do not run this test against the original implementation
						as it does not verify the validity of archive, resulting
						in a segment violation.
 */
void TValidateInstantiationTest::ArchiveParamInvalid()
{
	errno = B_OK;
	CPPUNIT_ASSERT(!validate_instantiation(NULL, gClassName));
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	validate_instantiation(BMessage* archive, const char* className)
	@case				Valid archive and className with no string field "class"
						in archive
	@param archive		Valid BMessage pointer
	@param className	Valid C-string
	@results			Returns false.
						errno is set to B_MISMATCHED_VALUES.
 */
void TValidateInstantiationTest::ClassFieldEmpty()
{
	errno = B_OK;
	BMessage Archive;
	CPPUNIT_ASSERT(!validate_instantiation(&Archive, gClassName));
	CPPUNIT_ASSERT(errno == B_MISMATCHED_VALUES);
}
//------------------------------------------------------------------------------
/**
	validate_instantiation(BMessage* archive, const char* className)
	@case				Valid archive with string field "class"; className does
						not match the content of "class"
	@param archive		Valid BMessage pointer with string field "class"
	@param className	Valid C-string which does not match archive field
						"class"
	@results			Returns false.
						errno is set to B_MISMATCHED_VALUES.
 */
void TValidateInstantiationTest::ClassFieldBogus()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gClassName);
	CPPUNIT_ASSERT(!validate_instantiation(&Archive, gBogusClassName));
	CPPUNIT_ASSERT(errno == B_MISMATCHED_VALUES);
}
//------------------------------------------------------------------------------
/**
	validate_instantiation(BMessage* archive, const char* className)
	@case				All parameters valid
	@param archive		Valid BMessage pointer with string field "class"
						containing a class name
	@param className	Valid C-string which matches contents of archive field
						"class"
	@results			Returns true.
						errno is set to B_OK.
 */
void TValidateInstantiationTest::AllValid()
{
	errno = B_OK;
	BMessage Archive;
	Archive.AddString("class", gClassName);
	CPPUNIT_ASSERT(validate_instantiation(&Archive, gClassName));
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------
CppUnit::Test* TValidateInstantiationTest::Suite()
{
	CppUnit::TestSuite* SuiteOfTests = new CppUnit::TestSuite;
#if !defined(TEST_R5)
	ADD_TEST(SuiteOfTests, TValidateInstantiationTest, AllParamsInvalid);
#endif
	ADD_TEST(SuiteOfTests, TValidateInstantiationTest, ClassNameParamInvalid);
#if !defined(TEST_R5)
	ADD_TEST(SuiteOfTests, TValidateInstantiationTest, ArchiveParamInvalid);
#endif
	ADD_TEST(SuiteOfTests, TValidateInstantiationTest, ClassFieldEmpty);
	ADD_TEST(SuiteOfTests, TValidateInstantiationTest, ClassFieldBogus);
	ADD_TEST(SuiteOfTests, TValidateInstantiationTest, AllValid);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */



