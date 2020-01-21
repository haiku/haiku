//------------------------------------------------------------------------------
//	BHandlerTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Handler.h>
#include <Looper.h>
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "BHandlerTester.h"
#include "LockLooperTestCommon.h"
// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	BHandler(const char* name)
	@case			Construct with NULL name
	@param	name	NULL
	@results		BHandler::Name() should return NULL
 */
void TBHandlerTester::BHandler1()
{
	BHandler Handler((const char*)NULL);
	CPPUNIT_ASSERT(Handler.Name() == NULL);
}
//------------------------------------------------------------------------------
/**
	BHandler(const char* name)
	@case			Construct with valid name
	@param	name	valid string
	@results		BHandler::Name() returns name
 */
void TBHandlerTester::BHandler2()
{
	BHandler Handler("name");
	CPPUNIT_ASSERT(string("name") == Handler.Name());
}
//------------------------------------------------------------------------------
/**
	BHandler(BMessage* archive)
	@case			archive is valid and has field "_name"
	@param	archive	valid BMessage pointer
	@results		BHandler::Name() returns _name
 */
void TBHandlerTester::BHandler3()
{
	BMessage Archive;
	Archive.AddString("_name", "the name");
	BHandler Handler(&Archive);
	CPPUNIT_ASSERT(string("the name") == Handler.Name());
}
//------------------------------------------------------------------------------
/**
	BHandler(BMessage* archive)
	@case			archive is valid, but has no field "_name"
	@param	archive	valid BMessage pointer
	@results		BHandler::Name() returns NULL
 */
void TBHandlerTester::BHandler4()
{
	BMessage Archive;
	BHandler Handler(&Archive);
	CPPUNIT_ASSERT(Handler.Name() == NULL);
}
//------------------------------------------------------------------------------
/**
	BHandler(BMessage* archive)
	@case			archive is NULL
	@param	archive	NULL
	@results		BHandler::Name() returns NULL
	@note			This test is not enabled against the original implementation
					as it doesn't check for a NULL parameter and seg faults.
 */

void TBHandlerTester::BHandler5()
{
#if !defined(TEST_R5)
	BHandler Handler((BMessage*)NULL);
	CPPUNIT_ASSERT(Handler.Name() == NULL);
#endif
}
//------------------------------------------------------------------------------
/**
	Archive(BMessage* data, bool deep = true)
	@case			data is NULL, deep is false
	@param	data	NULL
	@param	deep	false
	@results		Returns B_BAD_VALUE
	@note			This test is not enabled against the original implementation
					as it doesn't check for NULL parameters and seg faults
 */
void TBHandlerTester::Archive1()
{
#if !defined(TEST_R5)
	BHandler Handler;
	CPPUNIT_ASSERT(Handler.Archive(NULL, false) == B_BAD_VALUE);
#endif
}
//------------------------------------------------------------------------------
/**
	Archive(BMessage* data, bool deep = true)
	@case			data is NULL, deep is true
	@param	data	NULL
	@param	deep	false
	@results		Returns B_BAD_VALUE
	@note			This test is not enabled against the original implementation
					as it doesn't check for NULL parameters and seg faults
 */
void TBHandlerTester::Archive2()
{
#if !defined(TEST_R5)
	BHandler Handler;
	CPPUNIT_ASSERT(Handler.Archive(NULL) == B_BAD_VALUE);
#endif
}
//------------------------------------------------------------------------------
/**
	Archive(BMessage* data, bool deep = true)
	@case			data is valid, deep is false
	@param	data	valid BMessage pointer
	@param	deep	false
	@results		Returns B_OK
					Resultant archive has string field labeled "_name"
					Field "_name" contains the string "a name"
					Resultant archive has string field labeled "class"
					Field "class" contains the string "BHandler"
 */
void TBHandlerTester::Archive3()
{
	BMessage Archive;
	BHandler Handler("a name");
	CPPUNIT_ASSERT(Handler.Archive(&Archive, false) == B_OK);

	const char* data;
	CPPUNIT_ASSERT(Archive.FindString("_name", &data) == B_OK);
	CPPUNIT_ASSERT(string("a name") == data);
	CPPUNIT_ASSERT(Archive.FindString("class", &data) == B_OK);
	CPPUNIT_ASSERT(string("BHandler") == data);
}
//------------------------------------------------------------------------------
/**
	Archive(BMessage *data, bool deep = true)
	@case			data is valid, deep is true
	@param	data	valid BMessage pointer
	@param	deep	true
	@results		Returns B_OK
					Resultant archive has string field labeled "_name"
					Field "_name" contains the string "a name"
					Resultant archive has string field labeled "class"
					Field "class" contains the string "BHandler"
 */
void TBHandlerTester::Archive4()
{
	BMessage Archive;
	BHandler Handler("another name");
	CPPUNIT_ASSERT(Handler.Archive(&Archive) == B_OK);

	const char* data;
	CPPUNIT_ASSERT(Archive.FindString("_name", &data) == B_OK);
	CPPUNIT_ASSERT(string("another name") == data);
	CPPUNIT_ASSERT(Archive.FindString("class", &data) == B_OK);
	CPPUNIT_ASSERT(string("BHandler") == data);
}
//------------------------------------------------------------------------------
/**
	Instantiate(BMessage* data)
	@case			data is NULL
	@param	data	NULL
	@results
	@note			This test is not enabled against the original implementation
					as it doesn't check for NULL parameters and seg faults
 */
void TBHandlerTester::Instantiate1()
{
#if !defined(TEST_R5)
	CPPUNIT_ASSERT(BHandler::Instantiate(NULL) == NULL);
	CPPUNIT_ASSERT(errno == B_BAD_VALUE);
#endif
}
//------------------------------------------------------------------------------
/**
	Instantiate(BMessage* data)
	@case			data is valid, has field "_name"
	@param	data	Valid BMessage pointer with string field "class" containing
					string "BHandler" and with string field "_name" containing
					string "a name"
	@results		BHandler::Name() returns "a name"
 */
void TBHandlerTester::Instantiate2()
{
	BMessage Archive;
	Archive.AddString("class", "BHandler");
	Archive.AddString("_name", "a name");

	BHandler* Handler =
		dynamic_cast<BHandler*>(BHandler::Instantiate(&Archive));
	CPPUNIT_ASSERT(Handler != NULL);
	CPPUNIT_ASSERT(string("a name") == Handler->Name());
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------
/**
	Instantiate(BMessage *data)
	@case			data is valid, has no field "_name"
	@param	data	valid BMessage pointer with string field "class" containing
					string "BHandler"
	@results		BHandler::Name() returns NULL
 */

void TBHandlerTester::Instantiate3()
{
	BMessage Archive;
	Archive.AddString("class", "BHandler");

	BHandler* Handler =
		dynamic_cast<BHandler*>(BHandler::Instantiate(&Archive));
	CPPUNIT_ASSERT(Handler != NULL);
	CPPUNIT_ASSERT(Handler->Name() == NULL);
	CPPUNIT_ASSERT(errno == B_OK);
}
//------------------------------------------------------------------------------
/**
	SetName(const char* name)
	Name()
	@case			name is NULL
	@param	name	NULL
	@results		BHandler::Name() returns NULL
	
 */
void TBHandlerTester::SetName1()
{
	BHandler Handler("a name");
	CPPUNIT_ASSERT(string("a name") == Handler.Name());

	Handler.SetName(NULL);
	CPPUNIT_ASSERT(Handler.Name() == NULL);
}
//------------------------------------------------------------------------------
/**
	SetName(const char *name)
	Name()
	@case			name is valid
	@param	name	Valid string pointer
	@results		BHandler::Name returns name
 */
void TBHandlerTester::SetName2()
{
	BHandler Handler("a name");
	CPPUNIT_ASSERT(string("a name") == Handler.Name());

	Handler.SetName("another name");
	CPPUNIT_ASSERT(string("another name") == Handler.Name());
}
//------------------------------------------------------------------------------
/**
	Perform(perform_code d, void *arg)
	@case		feed meaningless data, should return B_NAME_NOT_FOUND
	@param	d	N/A
	@param	arg	NULL
	@results	Returns B_NAME_NOT_FOUND
 */
void TBHandlerTester::Perform1()
{
	BHandler Handler;
	CPPUNIT_ASSERT(Handler.Perform(0, NULL) == B_NAME_NOT_FOUND);
}
//------------------------------------------------------------------------------
/**
	FilterList();
	@case		Default constructed BHandler
	@results	FilterList() returns NULL
 */
void TBHandlerTester::FilterList1()
{
	BHandler Handler;
	CPPUNIT_ASSERT(!Handler.FilterList());
}
//------------------------------------------------------------------------------
Test* TBHandlerTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, BHandler1);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, BHandler2);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, BHandler3);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, BHandler4);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, BHandler5);

	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Archive1);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Archive2);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Archive3);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Archive4);

	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Instantiate1);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Instantiate2);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Instantiate3);

	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, SetName1);
	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, SetName2);

	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, Perform1);

	ADD_TEST4(BHandler, SuiteOfTests, TBHandlerTester, FilterList1);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */



