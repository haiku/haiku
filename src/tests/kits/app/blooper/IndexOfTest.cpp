//------------------------------------------------------------------------------
//	IndexOfTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "IndexOfTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	IndexOf(BHandler* handler)
	@case		handler is NULL
	@param		handler	NULL
	@results	IndexOf() returns B_ERROR
 */
void TIndexOfTest::IndexOfTest1()
{
	BLooper Looper;
	CPPUNIT_ASSERT(Looper.IndexOf(NULL) == B_ERROR);
}
//------------------------------------------------------------------------------
/**
	IndexOf(BHandler* handler)
	@case		handler is valid, doesn't belong to this looper
	@param		handler	Valid BHandler pointer, not assigned to this looper
	@results	IndexOf() returns B_ERROR
 */
void TIndexOfTest::IndexOfTest2()
{
	BLooper Looper;
	BHandler Handler;
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler) == B_ERROR);
}
//------------------------------------------------------------------------------
/**
	IndexOf(BHandler* handler)
	@case		handler is valid, belongs to looper
	@param		handler	Valid BHandler pointer, assigned to this looper
	@results	IndexOf returns 1
 */
void TIndexOfTest::IndexOfTest3()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler) == 1);
}
//------------------------------------------------------------------------------
/**
	IndexOf(BHandler* handler)
	@case		handler is valid, one of many added and removed
	@param		handler	Valid BHandler pointer, assigned to this looper
	@results	IndexOf() returns correct index for each handler
 */
void TIndexOfTest::IndexOfTest4()
{
	BLooper Looper;
	BHandler Handler1;
	BHandler Handler2;
	BHandler Handler3;
	BHandler Handler4;
	BHandler Handler5;

	Looper.AddHandler(&Handler1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	Looper.AddHandler(&Handler2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	Looper.AddHandler(&Handler3);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
	Looper.AddHandler(&Handler4);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == 4);
	Looper.AddHandler(&Handler5);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == 4);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == 5);

	// Now we remove them
	Looper.RemoveHandler(&Handler5);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == 4);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == B_ERROR);
	Looper.RemoveHandler(&Handler4);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == B_ERROR);
	Looper.RemoveHandler(&Handler3);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == B_ERROR);
	Looper.RemoveHandler(&Handler2);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == B_ERROR);
	Looper.RemoveHandler(&Handler1);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == B_ERROR);
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == B_ERROR);
}
//------------------------------------------------------------------------------
/**
	IndexOf(BHandler* handler)
	@case		handler is valid, looper is unlocked
	@param		handler	Valid BHandler pointer, assigned to this looper
	@results	IndexOf returns 1.  Debugger message "Looper must be locked
				before calling IndexOf."
 */
void TIndexOfTest::IndexOfTest5()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	Looper.Unlock();
	CPPUNIT_ASSERT(Looper.IndexOf(&Handler) == 1);
}
//------------------------------------------------------------------------------
/**
	IndexOf(BHandler* handler)
	@case		handler is "this"
	@param		handler	The looper's this pointer
	@result		IndexOf() returns 0
 */
void TIndexOfTest::IndexOfTest6()
{
	BLooper Looper;
	CPPUNIT_ASSERT(Looper.IndexOf(&Looper) == 0);
}
//------------------------------------------------------------------------------
TestSuite* TIndexOfTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::IndexOf(BHandler*)");

	ADD_TEST4(BLooper, suite, TIndexOfTest, IndexOfTest1);
	ADD_TEST4(BLooper, suite, TIndexOfTest, IndexOfTest2);
	ADD_TEST4(BLooper, suite, TIndexOfTest, IndexOfTest3);
	ADD_TEST4(BLooper, suite, TIndexOfTest, IndexOfTest4);
	ADD_TEST4(BLooper, suite, TIndexOfTest, IndexOfTest5);
	ADD_TEST4(BLooper, suite, TIndexOfTest, IndexOfTest6);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


