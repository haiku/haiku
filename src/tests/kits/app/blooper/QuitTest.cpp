//------------------------------------------------------------------------------
//	QuitTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "QuitTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	Quit()
	@case	Looper is unlocked
	@result	Prints message "ERROR - you must Lock a looper before calling
			Quit(), team=%ld, looper=%s\n"
 */
void TQuitTest::QuitTest1()
{
	BLooper* Looper = new BLooper;
	Looper->Unlock();
	Looper->Quit();
}
//------------------------------------------------------------------------------
TestSuite* TQuitTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::Quit()");
	ADD_TEST4(BLooper, suite, TQuitTest, QuitTest1);
	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

