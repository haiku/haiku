//------------------------------------------------------------------------------
//	LooperForThreadTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LooperForThreadTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	LooperForThread(thread_id)
	@case		tid is valid
 */
void TLooperForThreadTest::LooperForThreadTest1()
{
	BLooper* Looper = new BLooper;
	thread_id tid = Looper->Run();
	CPPUNIT_ASSERT(Looper == BLooper::LooperForThread(tid));
	Looper->Lock();
	Looper->Quit();
}
//------------------------------------------------------------------------------
/**
	LooperForThread(thread_id)
	@case		tid is not valid
 */
void TLooperForThreadTest::LooperForThreadTest2()
{
	CPPUNIT_ASSERT(BLooper::LooperForThread(find_thread(NULL)) == NULL);
}
//------------------------------------------------------------------------------
#ifdef ADD_TEST
#undef ADD_TEST
#endif
#define ADD_TEST(__test_name__)	\
	ADD_TEST4(BLooper, suite, TLooperForThreadTest, __test_name__)
TestSuite* TLooperForThreadTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::LooperForTest(thread_id)");

	ADD_TEST(LooperForThreadTest1);
	ADD_TEST(LooperForThreadTest2);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

