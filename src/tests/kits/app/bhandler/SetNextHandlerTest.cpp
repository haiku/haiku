//------------------------------------------------------------------------------
//	SetNextHandlerTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "SetNextHandlerTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Clear the next handler
	@param	handler	Valid BHandler pointer, then NULL
	@results		NextHandler() returns Handler2, then NULL
 */
void TSetNextHandlerTest::SetNextHandler0()
{
	BLooper Looper;
	BHandler Handler1;
	BHandler Handler2;

	Looper.AddHandler(&Handler1);
	Looper.AddHandler(&Handler2);

	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Handler2);

	Handler1.SetNextHandler(NULL);
	CPPUNIT_ASSERT(!Handler1.NextHandler());
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 do not belong to a BLooper
	@param	handler	Valid BHandler pointer
	@results		NextHandler() returns NULL
					debug message "handler must belong to looper before setting
					NextHandler"
 */
void TSetNextHandlerTest::SetNextHandler1()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == NULL);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 belongs to a unlocked BLooper, Handler2 does not
	@param	handler	Valid BHandler pointer
	@results		NextHandler() returns BLooper
					debug message "The handler's looper must be locked before
					setting NextHandler"
 */
void TSetNextHandlerTest::SetNextHandler2()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper;
	Looper.AddHandler(&Handler1);
	Looper.Unlock();
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 belongs to a locked BLooper, Handler2 does not
	@param	handler	Valid BHandler pointer
	@results		NextHandler() returns BLooper
					debug message "The handler and its NextHandler must have
					the same looper"
 */
void TSetNextHandlerTest::SetNextHandler3()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper;
	Looper.AddHandler(&Handler1);
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler2 belongs to a unlocked BLooper, Handler1 does not
	@param	handler	Valid BHandler pointer
	@results		NextHandler() returns NULL
					debug message "handler must belong to looper before setting
					NextHandler"
 */
void TSetNextHandlerTest::SetNextHandler4()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper;
	Looper.AddHandler(&Handler2);
	Looper.Unlock();
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == NULL);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler2 belongs to a locked BLooper, Handler1 does not
	@param	handler	Valid BHandler pointer
	@results		NextHandler() returns NULL
					debug message "handler must belong to looper before setting
					NextHandler"
 */
void TSetNextHandlerTest::SetNextHandler5()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper;
	Looper.AddHandler(&Handler2);
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == NULL);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 belong to different unlocked BLoopers
	@param	handler	Valid BHandler pointer
	@results		Returns BLooper;
					debug message "The handler and its NextHandler must have the
					same looper"
 */
void TSetNextHandlerTest::SetNextHandler6()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper1;
	BLooper Looper2;
	Looper1.AddHandler(&Handler1);
	Looper2.AddHandler(&Handler2);
	Looper1.Unlock();
	Looper2.Unlock();
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 belong to different BLoopers;
					Handler1's is locked; Handler2's is not
	@param	handler	Valid BHandler pointer
	@result			Returns BLooper;
					debug message "The handler and its NextHandler must have the
					same looper"
 */
void TSetNextHandlerTest::SetNextHandler7()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper1;
	BLooper Looper2;
	Looper1.AddHandler(&Handler1);
	Looper2.AddHandler(&Handler2);
	Looper2.Unlock();
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 belong to different BLoopers;
					Handler1's is unlocked; Handler2's is locked
	@param	handler	Valid BHandler pointer
	@results		Returns BLooper
					debug message "The handler and its NextHandler must have the
					same looper"
 */
void TSetNextHandlerTest::SetNextHandler8()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper1;
	BLooper Looper2;
	Looper1.AddHandler(&Handler1);
	Looper2.AddHandler(&Handler2);
	Looper1.Unlock();
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 belong to different locked BLoopers
	@param	handler	Valid BHandler pointer
	@results		Returns BLooper
					debug message "The handler and its NextHandler must have the
					same looper"
 */
void TSetNextHandlerTest::SetNextHandler9()
{
	DEBUGGER_ESCAPE;

	BHandler Handler1;
	BHandler Handler2;
	BLooper Looper1;
	BLooper Looper2;
	Looper1.AddHandler(&Handler1);
	Looper2.AddHandler(&Handler2);
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 belong to the same unlocked BLooper
	@param	handler	Valid BHandler pointer
	@results		Returns Handler2
					debug message "The handler's looper must be locked before
					setting NextHandler"
	@note			R5 implementation allows the next handler to be set anyway;
					we do the same.
 */
void TSetNextHandlerTest::SetNextHandler10()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BHandler Handler1;
	BHandler Handler2;
	Looper.AddHandler(&Handler1);
	Looper.AddHandler(&Handler2);
	Looper.Unlock();
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper);
}
//------------------------------------------------------------------------------
/**
	SetNextHandler(BHandler* handler);
	NextHandler();
	@case			Handler1 and Handler2 belong to the same locked BLooper
	@param	handler	Valid BHandler pointer
	@results		Returns Handler2
 */
void TSetNextHandlerTest::SetNextHandler11()
{
	BLooper Looper;
	BHandler Handler1;
	BHandler Handler2;
	Looper.AddHandler(&Handler1);
	Looper.AddHandler(&Handler2);
	Handler1.SetNextHandler(&Handler2);
	CPPUNIT_ASSERT(Handler1.NextHandler() == &Handler2);
}
//------------------------------------------------------------------------------
Test* TSetNextHandlerTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::SetNextHandler");

	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler0);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler1);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler2);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler3);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler4);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler5);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler6);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler7);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler8);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler9);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler10);
	ADD_TEST4(BHandler, SuiteOfTests, TSetNextHandlerTest, SetNextHandler11);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


