//------------------------------------------------------------------------------
//	LockTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Clipboard.h>

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LockTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	bool Lock()
	@case 1
	@results		Lock() returns true
 */
void LockTester::Lock1()
{
  BClipboard clip("Lock1");

  CHK(clip.Lock());
}

static int32 LockTest2(void *data)
{
  BClipboard *clip = (BClipboard *)data;
  clip->Lock();
  snooze(3000000); //Should be 3 seconds
  delete clip;
  return 0;
}
/*
	bool Lock()
	@case 2
	@results		Lock() returns false
 */
void LockTester::Lock2()
{
  BClipboard *clip = new BClipboard("Lock2");

  /* This method isn't guaranteed to work, but *should* work.
     Spawn a thread which locks the clipboard, waits several seconds
     and then deletes the locked clipboard.  After spawning
     the thread, the main thread should keep checking if the clipboard
     is locked.  Once it is locked, the thread tries to lock it and gets
     blocked.  Once the clipboard is deleted, it should return false.
     */
  thread_id thread = spawn_thread(LockTest2,"locktest",B_NORMAL_PRIORITY,clip);

  while(!clip->IsLocked());
  CHK(clip->Lock() == false);
  kill_thread(thread);
}

/*
	bool IsLocked()
	@case 1
	@results		IsLocked() returns true
 */
void LockTester::IsLocked1()
{
  BClipboard clip("IsLocked1");

  clip.Lock();
  CHK(clip.IsLocked());
}

/*
	bool IsLocked()
	@case 2
	@results		IsLocked() returns false
 */
void LockTester::IsLocked2()
{
  BClipboard clip("IsLocked2");

  CHK(!clip.IsLocked());
}

/*
	void Unlock()
	@case 1
	@results		IsLocked() returns false
 */
void LockTester::Unlock1()
{
  BClipboard clip("Unlock1");

  clip.Lock();
  CHK(clip.IsLocked());
  clip.Unlock();
  CHK(!clip.IsLocked());
}

Test* LockTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BClipboard, SuiteOfTests, LockTester, Lock1);
	ADD_TEST4(BClipboard, SuiteOfTests, LockTester, Lock2);
	ADD_TEST4(BClipboard, SuiteOfTests, LockTester, IsLocked1);
	ADD_TEST4(BClipboard, SuiteOfTests, LockTester, IsLocked2);
	ADD_TEST4(BClipboard, SuiteOfTests, LockTester, Unlock1);

	return SuiteOfTests;
}



