//------------------------------------------------------------------------------
//	HandlerAtTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "HandlerAtTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	HandlerAt(int32 index)
	@case		No handlers added, check for looper itself
	@param		index	0
	@results	HandlerAt() returns pointer to Looper.
 */
void THandlerAtTest::HandlerAtTest1()
{
	BLooper Looper;
	CPPUNIT_ASSERT(Looper.HandlerAt(0) == &Looper);
}
//------------------------------------------------------------------------------
/**
	HandlerAt(int32 index)
	@case		Index out of range (CountHandlers() + 1)
	@param		index	1 & -1
	@results	HandlerAt() returns NULL
 */
void THandlerAtTest::HandlerAtTest2()
{
	BLooper Looper;
	CPPUNIT_ASSERT(Looper.HandlerAt(1) == NULL);
	CPPUNIT_ASSERT(Looper.HandlerAt(-1) == NULL);
}
//------------------------------------------------------------------------------
/**
	HandlerAt(int32 index)
	@case		Several handlers added, checked against expected indices
	@param		index	Various
	@results	
 */
#define CREATE_AND_ADD(XHANDLER)	\
	BHandler XHANDLER;				\
	Looper.AddHandler(&XHANDLER);
void THandlerAtTest::HandlerAtTest3()
{
	BLooper Looper;
	CREATE_AND_ADD(Handler1);
	CREATE_AND_ADD(Handler2);
	CREATE_AND_ADD(Handler3);

	CPPUNIT_ASSERT(Looper.HandlerAt(0) == &Looper);
	CPPUNIT_ASSERT(Looper.HandlerAt(1) == &Handler1);
	CPPUNIT_ASSERT(Looper.HandlerAt(2) == &Handler2);
	CPPUNIT_ASSERT(Looper.HandlerAt(3) == &Handler3);
}
//------------------------------------------------------------------------------
/**
	HandlerAt(int32 index)
	@case		Looper is not locked
	@param		index	0
	@results	
 */
void THandlerAtTest::HandlerAtTest4()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	Looper.Unlock();
	CPPUNIT_ASSERT(Looper.HandlerAt(0) == &Looper);
}
//------------------------------------------------------------------------------
TestSuite* THandlerAtTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::HandlerAt(int32)");

	ADD_TEST4(BLooper, suite, THandlerAtTest, HandlerAtTest1);
	ADD_TEST4(BLooper, suite, THandlerAtTest, HandlerAtTest2);
	ADD_TEST4(BLooper, suite, THandlerAtTest, HandlerAtTest3);
	ADD_TEST4(BLooper, suite, THandlerAtTest, HandlerAtTest4);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


