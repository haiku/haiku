/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <Application.h>
#include <Clipboard.h>
#include <Handler.h>

#include <TestShell.h>
#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class RWHandler : public BHandler {
public:
	RWHandler()
		: BHandler()
	{
		fClipboardModified = false;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case B_CLIPBOARD_CHANGED:
				fClipboardModified = true;
				break;
			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

	bool ClipboardModified() { return fClipboardModified; }

private:
	bool fClipboardModified;
};


class ClipboardReadWriteTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ClipboardReadWriteTest);
	CPPUNIT_TEST(NotLocked_Clear_Fails);
	CPPUNIT_TEST(Locked_Clear_Succeeds);
	CPPUNIT_TEST(NotLocked_Revert_Fails);
	CPPUNIT_TEST(Locked_Revert_RestoresPreviousData);
	CPPUNIT_TEST(NotLocked_Commit_Fails);
	CPPUNIT_TEST(Locked_Commit_UpdatesSystemData);
	CPPUNIT_TEST(NotLocked_Data_Fails);
	CPPUNIT_TEST(Locked_Data_ReturnsMessageWithCorrectData);
	CPPUNIT_TEST(Empty_DataSource_ReturnsInvalidMessenger);
	CPPUNIT_TEST(ModifiedNotLocked_DataSource_ReturnsInvalidMessenger);
	CPPUNIT_TEST(Committed_DataSource_ReturnsValidMessenger);
	CPPUNIT_TEST(Target_StartWatching_NotifiesOnChange);
	CPPUNIT_TEST(WatchedMessenger_StopWatching_Succeeds);
	CPPUNIT_TEST(NotWatchedMessenger_StopWatching_Fails);
	CPPUNIT_TEST_SUITE_END();

public:
	void NotLocked_Clear_Fails()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Clear1");

#ifdef TEST_R5
		CPPUNIT_ASSERT(false);
#endif
		CPPUNIT_ASSERT_DEBUGGER(clip.Clear());
	}

	void Locked_Clear_Succeeds()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Clear2");
		BMessage* data;

		if (clip.Lock()) {
			CPPUNIT_ASSERT_EQUAL(B_OK, clip.Clear());
			if ((data = clip.Data()))
				CPPUNIT_ASSERT(data->IsEmpty());
		}
	}

	void NotLocked_Revert_Fails()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Revert1");

#ifdef TEST_R5
		CPPUNIT_ASSERT(false);
#endif
		CPPUNIT_ASSERT_DEBUGGER(clip.Revert());
	}

	void Locked_Revert_RestoresPreviousData()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Revert2");
		BMessage* data;
		char* str;
		ssize_t size;

		if (clip.Lock()) {
			clip.Clear();
			if ((data = clip.Data())) {
				data->AddData("text/plain", B_MIME_TYPE, "Revert2", 8);
				clip.Commit();
			}
			clip.Unlock();
		}

		if (clip.Lock()) {
			clip.Clear();
			if ((data = clip.Data()))
				data->AddData("text/plain", B_MIME_TYPE, "Foo", 4);
			CPPUNIT_ASSERT_EQUAL(B_OK, clip.Revert());
			if ((data = clip.Data())) {
				data->FindData("text/plain", B_MIME_TYPE, (const void**)&str, &size);
				CPPUNIT_ASSERT(strcmp(str, "Revert2") == 0);
			}
			clip.Unlock();
		}
	}

	void NotLocked_Commit_Fails()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Commit1");

#ifdef TEST_R5
		CPPUNIT_ASSERT(false);
#endif
		CPPUNIT_ASSERT_DEBUGGER(clip.Commit());
	}

	void Locked_Commit_UpdatesSystemData()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clipA("Commit2");
		BClipboard clipB("Commit2");
		BMessage* data;
		char* str;
		ssize_t size;

		if (clipA.Lock()) {
			clipA.Clear();
			if ((data = clipA.Data())) {
				data->AddData("text/plain", B_MIME_TYPE, "Commit2", 8);
				CPPUNIT_ASSERT_EQUAL(B_OK, clipA.Commit());
			}
			clipA.Unlock();
		}
		if (clipB.Lock()) {
			if ((data = clipB.Data())) {
				data->FindData("text/plain", B_MIME_TYPE, (const void**)&str, &size);
				CPPUNIT_ASSERT(strcmp(str, "Commit2") == 0);
			}
			clipB.Unlock();
		}
	}

	void NotLocked_Data_Fails()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Data1");

#ifdef TEST_R5
		CPPUNIT_ASSERT(false);
#endif
		CPPUNIT_ASSERT_DEBUGGER(clip.Data());
	}

	void Locked_Data_ReturnsMessageWithCorrectData()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("Data2");
		BMessage* data;
		char* str;
		ssize_t size;

		if (clip.Lock()) {
			clip.Clear();
			data = clip.Data();
			CPPUNIT_ASSERT(data);
			data->AddData("text/plain", B_MIME_TYPE, "Data2", 6);
			clip.Commit();
			clip.Unlock();
		}
		if (clip.Lock()) {
			data = clip.Data();
			CPPUNIT_ASSERT(data);
			data->FindData("text/plain", B_MIME_TYPE, (const void**)&str, &size);
			CPPUNIT_ASSERT(strcmp(str, "Data2") == 0);
			clip.Unlock();
		}
	}

	void Empty_DataSource_ReturnsInvalidMessenger()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("DataSource1");

		CPPUNIT_ASSERT(!clip.DataSource().IsValid());
	}

	void ModifiedNotLocked_DataSource_ReturnsInvalidMessenger()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clipA("DataSource2");
		BClipboard clipB("DataSource2");
		BMessage* data;

		if (clipA.Lock()) {
			clipA.Clear();
			if ((data = clipA.Data())) {
				data->AddData("text/plain", B_MIME_TYPE, "DataSource2", 12);
				clipA.Commit();
			}
			clipA.Unlock();
		}
		CPPUNIT_ASSERT(!clipB.DataSource().IsValid());
	}

	void Committed_DataSource_ReturnsValidMessenger()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clipA("DataSource3");
		BClipboard clipB("DataSource3");
		BMessage* data;

		if (clipA.Lock()) {
			clipA.Clear();
			if ((data = clipA.Data())) {
				data->AddData("text/plain", B_MIME_TYPE, "DataSource3", 12);
				clipA.Commit();
			}
			clipA.Unlock();
		}
		if (clipB.Lock()) {
			CPPUNIT_ASSERT(clipB.DataSource().IsValid());
			CPPUNIT_ASSERT(clipB.DataSource() == be_app_messenger);
		}
	}

	void Target_StartWatching_NotifiesOnChange()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("StartWatching1");
		BMessage* data;

		BLooper* looper = new BLooper();
		RWHandler handler;
		looper->AddHandler(&handler);
		looper->Run();
		BMessenger target(&handler);
		CPPUNIT_ASSERT_EQUAL(B_OK, clip.StartWatching(target));
		if (clip.Lock()) {
			clip.Clear();
			data = clip.Data();
			data->AddData("text/plain", B_MIME_TYPE, "StartWatching1", 15);
			clip.Commit();
			clip.Unlock();
		}
		snooze(100000);
		looper->Lock();
		looper->Quit();
		CPPUNIT_ASSERT(handler.ClipboardModified());
		clip.StopWatching(target);
	}

	void WatchedMessenger_StopWatching_Succeeds()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("StopWatching1");
		CPPUNIT_ASSERT_EQUAL(B_OK, clip.StartWatching(be_app_messenger));
		CPPUNIT_ASSERT_EQUAL(B_OK, clip.StopWatching(be_app_messenger));
	}

	void NotWatchedMessenger_StopWatching_Fails()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("StopWatching2");
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, clip.StopWatching(be_app_messenger));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ClipboardReadWriteTest, getTestSuiteName());
