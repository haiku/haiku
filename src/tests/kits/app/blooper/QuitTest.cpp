//------------------------------------------------------------------------------
//	QuitTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <Messenger.h>

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
void
TQuitTest::QuitTest1()
{
	BLooper* looper = new BLooper;
	looper->Unlock();
	looper->Quit();
}


void
TQuitTest::QuitTest2()
{
	BLooper* looper = new BLooper;
	looper->Run();

	BMessage reply;
	BMessenger(looper).SendMessage(B_QUIT_REQUESTED, &reply);
}


//------------------------------------------------------------------------------
TestSuite*
TQuitTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::Quit()");
	ADD_TEST4(BLooper, suite, TQuitTest, QuitTest1);
	ADD_TEST4(BLooper, suite, TQuitTest, QuitTest2);
	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

