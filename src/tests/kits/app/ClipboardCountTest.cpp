/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Clipboard.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class ClipboardCountTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ClipboardCountTest);
	CPPUNIT_TEST(Empty_LocalCount_Zero);
	CPPUNIT_TEST(AfterCommit_LocalCount_Increments);
	CPPUNIT_TEST(MultipleInstances_LocalCount_BothIncrement);
	CPPUNIT_TEST(MultipleInstances_LocalCount_IndependentIncrements);
	CPPUNIT_TEST(MultipleInstances_LocalCount_SharedIncrements);
	CPPUNIT_TEST(DifferentClipboards_LocalCount_Independent);
	CPPUNIT_TEST(Empty_SystemCount_Zero);
	CPPUNIT_TEST(MultipleInstances_SystemCount_SharedIncrement);
	CPPUNIT_TEST(DifferentClipboards_SystemCount_Independent);
	CPPUNIT_TEST_SUITE_END();

public:
	void Empty_LocalCount_Zero()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("LocalCount1");

		CPPUNIT_ASSERT_EQUAL(0, clip.LocalCount());
	}

	void AfterCommit_LocalCount_Increments()
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

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clip.LocalCount());
	}

	void MultipleInstances_LocalCount_BothIncrement()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clipA("LocalCount3");
		BClipboard clipB("LocalCount3");
		BMessage *data;

		uint32 oldCount = clipA.SystemCount();

		CPPUNIT_ASSERT(clipB.Lock());
		clipB.Clear();
		if ((data = clipB.Data())) {
			data->AddData("text/plain", B_MIME_TYPE, "LocalCount3", 12);
			clipB.Commit();
		}
		clipB.Unlock();

		CPPUNIT_ASSERT(clipA.Lock());
		clipA.Unlock();

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipA.LocalCount());
		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipB.LocalCount());
	}

	void MultipleInstances_LocalCount_IndependentIncrements()
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

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipA.LocalCount());
		CPPUNIT_ASSERT_EQUAL(oldCount + 2, clipB.LocalCount());
	}

	void MultipleInstances_LocalCount_SharedIncrements()
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

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipA.LocalCount());
		CPPUNIT_ASSERT_EQUAL(oldCount + 2, clipB.LocalCount());
	}

	void DifferentClipboards_LocalCount_Independent()
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

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipA.LocalCount());
		CPPUNIT_ASSERT_EQUAL(0, clipB.LocalCount());
	}

	void Empty_SystemCount_Zero()
	{
		BApplication app("application/x-vnd.clipboardtest");
		BClipboard clip("SystemCount1");

		CPPUNIT_ASSERT_EQUAL(0, clip.SystemCount());
	}

	void MultipleInstances_SystemCount_SharedIncrement()
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

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipA.SystemCount());
		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipB.SystemCount());
	}

	void DifferentClipboards_SystemCount_Independent()
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

		CPPUNIT_ASSERT_EQUAL(oldCount + 1, clipA.SystemCount());
		CPPUNIT_ASSERT_EQUAL(0, clipB.SystemCount());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ClipboardCountTest, getTestSuiteName());
