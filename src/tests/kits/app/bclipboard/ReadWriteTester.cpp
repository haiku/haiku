//------------------------------------------------------------------------------
//	ReadWriteTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Clipboard.h>

#define CHK	CPPUNIT_ASSERT

#include <TestShell.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "ReadWriteTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	status_t Clear()
	@case 1
	@results		Clear() returns B_ERROR
 */
void ReadWriteTester::Clear1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Clear1");

#ifdef TEST_R5
  CHK(false);
#endif
  CPPUNIT_ASSERT_DEBUGGER(clip.Clear());
}

/*
	status_t Clear()
	@case 2
	@results		Clear() returns B_OK
				data message is empty
 */
void ReadWriteTester::Clear2()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Clear2");
  BMessage *data;

  if ( clip.Lock() )
  {
    CHK(clip.Clear() == B_OK);
    if ( (data = clip.Data()) )
      CHK(data->IsEmpty());
  }
}

/*
	status_t Revert()
	@case 1
	@results		Revert() returns B_ERROR
 */
void ReadWriteTester::Revert1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Revert1");

#ifdef TEST_R5
  CHK(false);
#endif
  CPPUNIT_ASSERT_DEBUGGER(clip.Revert());
}

/*
	status_t Revert()
	@case 2
	@results		Revert() returns B_OK
				data message is reset
 */
void ReadWriteTester::Revert2()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Revert2");
  BMessage *data;
  char *str;
  ssize_t size;

  if ( clip.Lock() )
  {
    clip.Clear();
    if ( (data = clip.Data()) )
    {
      data->AddData("text/plain",B_MIME_TYPE, "Revert2", 8);
      clip.Commit();
    }
    clip.Unlock();
  }

  if ( clip.Lock() )
  {
    clip.Clear();
    if ( (data = clip.Data()) )
    {
      data->AddData("text/plain",B_MIME_TYPE, "Foo", 4);
    }
    CHK(clip.Revert() == B_OK);
    if ( (data = clip.Data()) )
    {
      data->FindData("text/plain",B_MIME_TYPE, (const void **)&str, &size);
      CHK(strcmp(str,"Revert2") == 0);
    }
    clip.Unlock();
  }
}

/*
	status_t Commit()
	@case 1
	@results		Commit() returns B_ERROR
 */
void ReadWriteTester::Commit1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Commit1");

#ifdef TEST_R5
  CHK(false);
#endif
  CPPUNIT_ASSERT_DEBUGGER(clip.Commit());
}

/*
	status_t Commit()
	@case 2
	@results		Commit() returns B_OK
				data is uploaded from system
 */
void ReadWriteTester::Commit2()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clipA("Commit2");
  BClipboard clipB("Commit2");
  BMessage *data;
  char *str;
  ssize_t size;

  if ( clipA.Lock() )
  {
    clipA.Clear();
    if ( (data = clipA.Data()) )
    {
      data->AddData("text/plain",B_MIME_TYPE, "Commit2", 8);
      CHK(clipA.Commit() == B_OK);
    }
    clipA.Unlock();
  }
  if ( clipB.Lock() )
  {
    if ( (data = clipB.Data()) )
    {
      data->FindData("text/plain",B_MIME_TYPE, (const void **)&str, &size);
      CHK(strcmp(str,"Commit2") == 0);
    }
    clipB.Unlock();
  }
}

/*
	BMessage *Data()
	@case 1
	@results		Data() returns NULL
 */
void ReadWriteTester::Data1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Data1");

#ifdef TEST_R5
  CHK(false);
#endif
  CPPUNIT_ASSERT_DEBUGGER(clip.Data());
}

/*
	BMessage *Data()
	@case 2
	@results		Data() returns a message with correct data
 */
void ReadWriteTester::Data2()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("Data2");
  BMessage *data;
  char *str;
  ssize_t size;

  if ( clip.Lock() )
  {
    clip.Clear();
    data = clip.Data();
    CHK(data);
    data->AddData("text/plain",B_MIME_TYPE, "Data2", 6);
    clip.Commit();
    clip.Unlock();
  }
  if ( clip.Lock() )
  {
    data = clip.Data();
    CHK(data);
    data->FindData("text/plain",B_MIME_TYPE, (const void **)&str, &size);
    CHK(strcmp(str,"Data2") == 0);
    clip.Unlock();
  }
}

/*
	BMessenger DataSource()
	@case 1
	@results		DataSource returns and invalid BMessenger
 */
void ReadWriteTester::DataSource1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("DataSource1");

  CHK(!clip.DataSource().IsValid());
}

/*
	BMessenger DataSource()
	@case 2
	@results		DataSource returns an invalid BMessenger
 */
void ReadWriteTester::DataSource2()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clipA("DataSource2");
  BClipboard clipB("DataSource2");
  BMessage *data;

  if ( clipA.Lock() )
  {
    clipA.Clear();
    if ( (data = clipA.Data()) )
    {
      data->AddData("text/plain",B_MIME_TYPE, "DataSource2", 12);
      clipA.Commit();
    }
    clipA.Unlock();
  }
  CHK(!clipB.DataSource().IsValid());
}

/*
	BMessenger DataSource()
	@case 3
	@results		DataSource returns a valid BMessenger
 */
void ReadWriteTester::DataSource3()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clipA("DataSource3");
  BClipboard clipB("DataSource3");
  BMessage *data;

  if ( clipA.Lock() )
  {
    clipA.Clear();
    if ( (data = clipA.Data()) )
    {
      data->AddData("text/plain",B_MIME_TYPE, "DataSource3", 12);
      clipA.Commit();
    }
    clipA.Unlock();
  }
  if ( clipB.Lock() )
  {
    CHK(clipB.DataSource().IsValid());
    CHK(clipB.DataSource() == be_app_messenger);
  }
}

/*
	status_t StartWatching(BMessenger target)
	@case 1
	@results		return B_OK, target is notified when clipboard is written
 */
void ReadWriteTester::StartWatching1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("StartWatching1");
  BMessage *data;

  BLooper *looper = new BLooper();
  RWHandler handler;
  looper->AddHandler(&handler);
  looper->Run();
  BMessenger target(&handler);
  CHK(clip.StartWatching(target) == B_OK);
  if ( clip.Lock() )
  {
    clip.Clear();
    data = clip.Data();
    data->AddData("text/plain",B_MIME_TYPE, "StartWatching1", 15);
    clip.Commit();
    clip.Unlock();
  }
  snooze(100000);
  looper->Lock();
  looper->Quit();
  CHK(handler.ClipboardModified());
  clip.StopWatching(target);
}

/*
	status_t StopWatching(BMessenger target)
	@case 1
	@results		return B_OK
 */
void ReadWriteTester::StopWatching1()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("StopWatching1");
  if ( clip.StartWatching(be_app_messenger) == B_OK )
  {
    CHK(clip.StopWatching(be_app_messenger) == B_OK);
  }
}

/*
	status_t StopWatching(BMessenger target)
	@case 2
	@results		return B_BAD_VALUE
 */
void ReadWriteTester::StopWatching2()
{
  BApplication app("application/x-vnd.clipboardtest");
  BClipboard clip("StopWatching2");
  CHK(clip.StopWatching(be_app_messenger) == B_BAD_VALUE);
}

Test* ReadWriteTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Clear1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Clear2);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Revert1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Revert2);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Commit1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Commit2);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Data1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, Data2);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, DataSource1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, DataSource2);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, DataSource3);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, StartWatching1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, StopWatching1);
	ADD_TEST4(BClipboard, SuiteOfTests, ReadWriteTester, StopWatching2);

	return SuiteOfTests;
}

// RWHandler

// constructor
RWHandler::RWHandler()
		 : BHandler()
{
	fClipboardModified = false;
}

// MessageReceived
void
RWHandler::MessageReceived(BMessage *message)
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

// ClipboardModified
bool 
RWHandler::ClipboardModified()
{
	return fClipboardModified;
}

