//------------------------------------------------------------------------------
//	FindInstatiationFuncTester.cpp
//
/**
	Tests for find_instantiation_func(const char* name, const char* sig)
	@note	There are no tests for find_instantiation_func(const char*) as it
			simply calls through to the version which takes an explicit sig,
			setting that parameter to NULL.

			Here's the use case matrix:

						name		sig
						--------	--------
			case 1		NULL		NULL
			case 2		bogus		NULL
			case 3		NULL		bogus
			case 4		bogus		bogus
			case 5		local		NULL
			case 6		remote		NULL
			case 7		local		bogus
			case 8		remote		bogus
			case 9		local		good
			case 10		remote		good
 */
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "FindInstantiationFuncTester.h"
#include "LocalTestObject.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Both parameters NULL
	@param	name	NULL
	@param	sig		NULL
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case1()
{
	instantiation_func f = find_instantiation_func(NULL, NULL);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Bad name with NULL signature
	@param	name	Invalid class name
	@param	sig		NULL
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case2()
{
	instantiation_func f = find_instantiation_func(gInvalidClassName, NULL);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			NULL name with invalid signature
	@param	name	NULL class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case3()
{
	instantiation_func f = find_instantiation_func(NULL, gInvalidSig);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Both params are invalid
	@param	name	Invalid class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case4()
{
	instantiation_func f = find_instantiation_func(gInvalidClassName,
												   gInvalidSig);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a locally implemented class with a
					NULL signature
	@param	name	Valid local class name
	@param	sig		NULL signature
	@results		Returns valid function
 */
void TFindInstantiationFuncTester::Case5()
{
	instantiation_func f = find_instantiation_func(gLocalClassName, NULL);
	CPPUNIT_ASSERT(f != NULL);

	BMessage Archive;
	Archive.AddString("class", gLocalClassName);
	TIOTest* Test = dynamic_cast<TIOTest*>(f(&Archive));
	CPPUNIT_ASSERT(Test != NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a remotely implemented class with a
					NULL signature
	@param	name	Valid remote class name
	@param	sig		NULL signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case6()
{
	instantiation_func f = find_instantiation_func(gRemoteClassName, NULL);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a locally implemented class with an
					invalid signature
	@param	name	Valid local class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case7()
{
	instantiation_func f = find_instantiation_func(gLocalClassName,
												   gInvalidSig);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a remotely implemented class with an
					invalid signature
	@param	name	Valid remote class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case8()
{
	instantiation_func f = find_instantiation_func(gRemoteClassName,
												   gInvalidSig);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a locally implemented class with a
					valid signature
	@param	name	Valid local class name
	@param	sig		Valid signature
	@results		Returns valid function
	@note			This test is not currently used; can't obtain the local
					signature without a BApplication object (gLocalSig is a
					placeholder).
 */
void TFindInstantiationFuncTester::Case9()
{
	instantiation_func f = find_instantiation_func(gLocalClassName, gLocalSig);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a remotely implemented class with a
					valid signature
	@param	name	Valid remote class name
	@param	sig		Valid signature
	@results		Returns NULL
	@note			This case's results are because find_instantiation_func
					doesn't actually load anything in order to do its work.
 */
void TFindInstantiationFuncTester::Case10()
{
	instantiation_func f = find_instantiation_func(gRemoteClassName,
												   gRemoteSig);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Archive is NULL
	@param	Archive	NULL
 */
void TFindInstantiationFuncTester::Case1M()
{
	instantiation_func f = find_instantiation_func((BMessage*)NULL);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Bad name with NULL signature
	@param	name	Invalid class name
	@param	sig		NULL
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case2M()
{
	BMessage Archive;
	Archive.AddString("class", gInvalidClassName);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			NULL name with invalid signature
	@param	name	NULL class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case3M()
{
	BMessage Archive;
	Archive.AddString("add_on", gInvalidSig);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Both params are invalid
	@param	name	Invalid class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case4M()
{
	BMessage Archive;
	Archive.AddString("class", gInvalidClassName);
	Archive.AddString("add_on", gInvalidSig);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a locally implemented class with a
					NULL signature
	@param	name	Valid local class name
	@param	sig		NULL signature
	@results		Returns valid function
 */
void TFindInstantiationFuncTester::Case5M()
{
	BMessage Archive;
	Archive.AddString("class", gLocalClassName);

	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f != NULL);

	TIOTest* Test = dynamic_cast<TIOTest*>(f(&Archive));
	CPPUNIT_ASSERT(Test != NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a remotely implemented class with a
					NULL signature
	@param	name	Valid remote class name
	@param	sig		NULL signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case6M()
{
	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a locally implemented class with an
					invalid signature
	@param	name	Valid local class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case7M()
{
	BMessage Archive;
	Archive.AddString("class", gLocalClassName);
	Archive.AddString("add_on", gInvalidSig);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a remotely implemented class with an
					invalid signature
	@param	name	Valid remote class name
	@param	sig		Invalid signature
	@results		Returns NULL
 */
void TFindInstantiationFuncTester::Case8M()
{
	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	Archive.AddString("add_on", gInvalidSig);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a locally implemented class with a
					valid signature
	@param	name	Valid local class name
	@param	sig		Valid signature
	@results		Returns valid function
	@note			This test is not currently used; can't obtain the local
					signature without a BApplication object (gLocalSig is a
					placeholder).
 */
void TFindInstantiationFuncTester::Case9M()
{
	BMessage Archive;
	Archive.AddString("class", gLocalClassName);
	Archive.AddString("add_on", gLocalSig);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
/**
	find_instantiation_func(const char* name, const char* sig)
	@case			Valid name of a remotely implemented class with a
					valid signature
	@param	name	Valid remote class name
	@param	sig		Valid signature
	@results		Returns NULL
	@note			This case's results are because find_instantiation_func
					doesn't actually load anything in order to do its work.
 */
void TFindInstantiationFuncTester::Case10M()
{
	BMessage Archive;
	Archive.AddString("class", gRemoteClassName);
	Archive.AddString("add_on", gRemoteSig);
	instantiation_func f = find_instantiation_func(&Archive);
	CPPUNIT_ASSERT(f == NULL);
}
//------------------------------------------------------------------------------
CppUnit::Test* TFindInstantiationFuncTester::Suite()
{
	CppUnit::TestSuite* SuiteOfTests = new CppUnit::TestSuite;

	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case1);
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case2);
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case3);
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case4);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case5);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case6);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case7);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case8);	
//	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case9);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case10);	

	// BMessage using versions
#if !defined(TEST_R5)
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case1M);
#endif
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case2M);
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case3M);
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case4M);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case5M);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case6M);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case7M);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case8M);	
//	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case9M);	
	ADD_TEST(SuiteOfTests, TFindInstantiationFuncTester, Case10M);	

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */



