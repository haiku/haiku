/*
* Copyright 2002-2026, Haiku, Inc. All rights reserved.
* Distributed under the terms of the MIT License.
*/


#include <Application.h>
#include <Clipboard.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


static int32
LockTest(void *data)
{
	BClipboard *clip = (BClipboard *)data;
	clip->Lock();
	snooze(300000);
		// Should be .3 seconds
	delete clip;
	return 0;
}


class ClipboardLockTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ClipboardLockTest);
	CPPUNIT_TEST(Lock_Succeeds);
	CPPUNIT_TEST(BlockedThenDeleted_Lock_ReturnsFalse);
	CPPUNIT_TEST(Locked_IsLocked_ReturnsTrue);
	CPPUNIT_TEST(NotLocked_IsLocked_ReturnsFalse);
	CPPUNIT_TEST(Locked_Unlock_Unlocks);
	CPPUNIT_TEST_SUITE_END();
	
public:
	void Lock_Succeeds()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Lock1");
		
		CPPUNIT_ASSERT(clip.Lock());
	}

	void BlockedThenDeleted_Lock_ReturnsFalse()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard *clip = new BClipboard("Lock2");
		
		/*
		This method isn't guaranteed to work, but *should* work.
		Spawn a thread which locks the clipboard, waits .3 seconds
		and then deletes the locked clipboard.  After spawning
		the thread, the main thread waits .1 second.  It then tries to
		lock the clipboard and gets blocked.  Once the clipboard is deleted,
		it should return false.
		*/
		thread_id thread = spawn_thread(LockTest, "locktest", B_NORMAL_PRIORITY, clip);
		CPPUNIT_ASSERT(thread >= B_OK);
		resume_thread(thread);
		snooze(100000);
			// Should be .1 seconds
		CPPUNIT_ASSERT_EQUAL(false, clip->Lock());
	}

	void Locked_IsLocked_ReturnsTrue()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("IsLocked1");
		
		clip.Lock();
		CPPUNIT_ASSERT(clip.IsLocked());
	}

	void NotLocked_IsLocked_ReturnsFalse()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("IsLocked2");
		
		CPPUNIT_ASSERT(!clip.IsLocked());
	}

	void Locked_Unlock_Unlocks()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Unlock1");
		
		clip.Lock();
		CPPUNIT_ASSERT(clip.IsLocked());
		clip.Unlock();
		CPPUNIT_ASSERT(!clip.IsLocked());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ClipboardLockTest, getTestSuiteName());
