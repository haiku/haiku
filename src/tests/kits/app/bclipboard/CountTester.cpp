//------------------------------------------------------------------------------
//	CountTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Clipboard.h>

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "CountTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	LocalCount()
	@case 1
	@results		count == 0
 */
void CountTester::LocalCount1()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clip("LocalCount1");

	CHK(clip.LocalCount() == 0);
}

/*
	LocalCount()
	@case 2
	@results		count == +1
 */
void CountTester::LocalCount2()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clip("LocalCount2");
	BMessage *data;

	uint32 oldCount = clip.SystemCount();

	if (clip.Lock()) {
		clip.Clear();
		if ((data = clip.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "LocalCount2", 12);
			clip.Commit();
		}
		clip.Unlock();
	}

	CHK(clip.LocalCount() == oldCount + 1);
}

/*
	LocalCount()
	@case 3
	@results		both counts == +1
 */
void CountTester::LocalCount3()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clipA("LocalCount3");
	BClipboard clipB("LocalCount3");
	BMessage *data;

	uint32 oldCount = clipA.SystemCount();

	CHK(clipB.Lock());
	clipB.Clear();
	if ((data = clipB.Data())) {
		data->AddData("text/plain", B_MIME_TYPE, "LocalCount3", 12);
		clipB.Commit();
	}
	clipB.Unlock();

	CHK(clipA.Lock());
	clipA.Unlock();

	CHK(clipA.LocalCount() == oldCount + 1);
	CHK(clipB.LocalCount() == oldCount + 1);
}

/*
	LocalCount()
	@case 4
	@results	clipA.LocalCount() == +1
				clipB.LocalCount() == +2
 */
void CountTester::LocalCount4()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clipA("LocalCount4");
	BClipboard clipB("LocalCount4");
	BMessage *data;

	uint32 oldCount = clipA.SystemCount();

	if (clipB.Lock()) {
		clipB.Clear();
		if ((data = clipB.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "LocalCount4", 12);
			clipB.Commit();
		}
		clipB.Unlock();
	}

	if (clipA.Lock())
		clipA.Unlock();

	if (clipB.Lock()) {
		clipB.Clear();
		if ((data = clipB.Data())) {
			data->AddData("text/plain",B_MIME_TYPE,"LocalCount4",12);
			clipB.Commit();
		}
		clipB.Unlock();
	}

	CHK(clipA.LocalCount() == oldCount + 1);
	CHK(clipB.LocalCount() == oldCount + 2);
}

/*
	LocalCount()
	@case 5
	@results	clipA.LocalCount() == 1
				clipB.LocalCount() == 2
 */
void CountTester::LocalCount5()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clipA("LocalCount5");
	BClipboard clipB("LocalCount5");
	BMessage *data;

	uint32 oldCount = clipA.SystemCount();

	if (clipA.Lock()) {
		clipA.Clear();
		if ((data = clipA.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "LocalCount5", 12);
			clipA.Commit();
		}
		clipA.Unlock();
	}

	if (clipB.Lock()) {
		clipB.Clear();
		if ((data = clipB.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "LocalCount5", 12);
			clipB.Commit();
		}
		clipB.Unlock();
	}

	CHK(clipA.LocalCount() == oldCount + 1);
	CHK(clipB.LocalCount() == oldCount + 2);
}

/*
	LocalCount()
	@case 6
	@results	clipA.LocalCount() == +1
				clipB.LocalCount() == 0
 */
void CountTester::LocalCount6()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clipA("LocalCount6A");
	BClipboard clipB("LocalCount6B");
	BMessage *data;

	uint32 oldCount = clipA.SystemCount();

	if (clipA.Lock()) {
		clipA.Clear();
		if ((data = clipA.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "LocalCount6" ,12);
			clipA.Commit();
		}
		clipA.Unlock();
	}

	if (clipB.Lock())
		clipB.Unlock();

	CHK(clipA.LocalCount() == oldCount + 1);
	CHK(clipB.LocalCount() == 0);
}

/*
	SystemCount()
	@case 1
	@results		count == 0
 */
void CountTester::SystemCount1()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clip("SystemCount1");

	CHK(clip.SystemCount() == 0);
}

/*
	SystemCount()
	@case 2
	@results	clipA.SystemCount() == +1
				clipB.SystemCount() == +1
 */
void CountTester::SystemCount2()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clipA("SystemCount2");
	BClipboard clipB("SystemCount2");
	BMessage *data;

	uint32 oldCount = clipA.SystemCount();

	if (clipA.Lock()) {
		clipA.Clear();
		if ((data = clipA.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "SystemCount2", 12);
			clipA.Commit();
		}
		clipA.Unlock();
	}

	CHK(clipA.SystemCount() == oldCount + 1);
	CHK(clipB.SystemCount() == oldCount + 1);
}

/*
	SystemCount()
	@case 3
	@results	clipA.SystemCount() == +1
				clipB.SystemCount() == 0
 */
void CountTester::SystemCount3()
{
	BApplication app("application/x-vnd.clipboardtest");
	BClipboard clipA("SystemCount3A");
	BClipboard clipB("SystemCount3B");
	BMessage *data;

	uint32 oldCount = clipA.SystemCount();

	if (clipA.Lock()) {
		clipA.Clear();
		if ((data = clipA.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "SystemCount3", 12);
			clipA.Commit();
		}
		clipA.Unlock();
	}

	CHK(clipA.SystemCount() == oldCount + 1);
	CHK(clipB.SystemCount() == 0);
}

Test* CountTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, LocalCount1);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, LocalCount2);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, LocalCount3);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, LocalCount4);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, LocalCount5);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, LocalCount6);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, SystemCount1);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, SystemCount2);
	ADD_TEST4(BClipboard, SuiteOfTests, CountTester, SystemCount3);

	return SuiteOfTests;
}
