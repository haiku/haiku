/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Leorize, leorize+oss@disroot.org
 */


#include <ThreadedTestCase.h>
#include <MemoryRingIO.h>
#include <OS.h>
#include <TestUtils.h>
#include <ThreadedTestCaller.h>

#include <stdio.h>
#include <string.h>

#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>



#define BIG_PAYLOAD \
	"a really long string that can fill the buffer multiple times"
#define FULL_PAYLOAD "16 characters x"
#define SMALL_PAYLOAD "shorter"


static void
ReadCheck(BMemoryRingIO& ring, const void* cmp, size_t size)
{
	char* buffer = new char[size];
	memset(buffer, 0, size);
	size_t read;
	CPPUNIT_ASSERT(ring.ReadExactly(buffer, size, &read) == B_OK);
	CPPUNIT_ASSERT(read == size);
	CPPUNIT_ASSERT(memcmp(buffer, cmp, size) == 0);
	delete[] buffer;
}


class MemoryRingIOTest : public BThreadedTestCase {
public:
	MemoryRingIOTest(size_t bufferSize = 0)
		:
		fRing(bufferSize)
	{
	}

	void _DisableWriteOnEmptyBuffer()
	{
		CPPUNIT_ASSERT(fRing.InitCheck() == B_OK);

		while (fRing.BytesAvailable() > 0)
			fRing.WaitForWrite();

		/* snooze for sometime to ensure that the other thread entered
		 * WaitForRead().
		 */
		snooze(1000);
		/* this should unblock the other thread */
		fRing.SetWriteDisabled(true);
	}

	void _DisableWriteOnFullBuffer()
	{
		CPPUNIT_ASSERT(fRing.InitCheck() == B_OK);

		while (fRing.SpaceAvailable() > 0)
			fRing.WaitForRead();

		/* snooze for sometime to ensure that the other thread entered
		 * WaitForWrite().
		 */
		snooze(1000);
		/* this should unblock the other thread */
		fRing.SetWriteDisabled(true);
	}

	void TimeoutTest()
	{
		fRing.Clear();
		CPPUNIT_ASSERT(fRing.SetSize(0) == B_OK);
		bigtime_t start = system_time();
		const bigtime_t timeout = 100;

		CPPUNIT_ASSERT(fRing.WaitForRead(timeout) == B_TIMED_OUT);
		CPPUNIT_ASSERT(system_time() - start <= timeout + 10);

		start = system_time();
		CPPUNIT_ASSERT(fRing.WaitForWrite(timeout) == B_TIMED_OUT);
		CPPUNIT_ASSERT(system_time() - start <= timeout + 10);
	}

	void InvalidResizeTest()
	{
		CPPUNIT_ASSERT(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
		CPPUNIT_ASSERT(fRing.WriteExactly(FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
		CPPUNIT_ASSERT(fRing.SetSize(0) == B_BAD_VALUE);
	}

	void ReadWriteSingleTest()
	{
		CPPUNIT_ASSERT(fRing.SetSize(sizeof(BIG_PAYLOAD)) == B_OK);
		CPPUNIT_ASSERT(fRing.WriteExactly(BIG_PAYLOAD, sizeof(BIG_PAYLOAD)) == B_OK);
		ReadCheck(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD));

		CPPUNIT_ASSERT(fRing.SetSize(sizeof(FULL_PAYLOAD)) == B_OK);
		// the size of FULL_PAYLOAD is a power of two, so our ring
		// should be using the exact size.
		CPPUNIT_ASSERT(fRing.BufferSize() == sizeof(FULL_PAYLOAD));
		CPPUNIT_ASSERT(fRing.WriteExactly(FULL_PAYLOAD, sizeof(FULL_PAYLOAD)) == B_OK);
		ReadCheck(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD));

		CPPUNIT_ASSERT(fRing.SetSize(sizeof(SMALL_PAYLOAD)) == B_OK);
		CPPUNIT_ASSERT(fRing.WriteExactly(SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD)) == B_OK);
		ReadCheck(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD));
	}

	void BusyReaderTest()
	{
		CPPUNIT_ASSERT(fRing.InitCheck() == B_OK);

		char buffer[100];
		CPPUNIT_ASSERT(fRing.Read(buffer, sizeof(buffer)) == 0);
	}

	void BusyWriterTest()
	{
		CPPUNIT_ASSERT(fRing.InitCheck() == B_OK);
		CPPUNIT_ASSERT(fRing.BufferSize() < sizeof(BIG_PAYLOAD));

		CPPUNIT_ASSERT(fRing.WriteExactly(BIG_PAYLOAD, sizeof(BIG_PAYLOAD), NULL)
			== B_READ_ONLY_DEVICE); // FIXME: was B_DEVICE_FULL
	}

	void ReadTest()
	{
		CPPUNIT_ASSERT(fRing.InitCheck() == B_OK);

		ReadCheck(fRing, SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD));
		ReadCheck(fRing, FULL_PAYLOAD, sizeof(FULL_PAYLOAD));
		ReadCheck(fRing, BIG_PAYLOAD, sizeof(BIG_PAYLOAD));
	}

	void WriteTest()
	{
		CPPUNIT_ASSERT(fRing.InitCheck() == B_OK);

		CPPUNIT_ASSERT(fRing.WriteExactly(SMALL_PAYLOAD, sizeof(SMALL_PAYLOAD), NULL) == B_OK);
		CPPUNIT_ASSERT(fRing.WriteExactly(FULL_PAYLOAD, sizeof(FULL_PAYLOAD), NULL) == B_OK);
		CPPUNIT_ASSERT(fRing.WriteExactly(BIG_PAYLOAD, sizeof(BIG_PAYLOAD), NULL) == B_OK);
	}

	static CppUnit::Test*
	suite()
	{
		CppUnit::TestSuite* suite = new CppUnit::TestSuite("MemoryRingIOTest");
		BThreadedTestCaller<MemoryRingIOTest>* caller;

		MemoryRingIOTest* big = new MemoryRingIOTest(sizeof(BIG_PAYLOAD));
		caller = new BThreadedTestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: RW threaded, big buffer", big);
		caller->addThread("WR", &MemoryRingIOTest::WriteTest);
		caller->addThread("RD", &MemoryRingIOTest::ReadTest);
		suite->addTest(caller);

		MemoryRingIOTest* full = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
		caller = new BThreadedTestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: RW threaded, medium buffer", full);
		caller->addThread("WR", &MemoryRingIOTest::WriteTest);
		caller->addThread("RD", &MemoryRingIOTest::ReadTest);
		suite->addTest(caller);

		MemoryRingIOTest* small = new MemoryRingIOTest(sizeof(SMALL_PAYLOAD));
		caller = new BThreadedTestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: RW threaded, small buffer", small);
		caller->addThread("WR", &MemoryRingIOTest::WriteTest);
		caller->addThread("RD", &MemoryRingIOTest::ReadTest);
		suite->addTest(caller);

		MemoryRingIOTest* endWrite = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
		caller = new BThreadedTestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: RW threaded, reader set end reached on writer wait",
			endWrite);
		caller->addThread("WR #1", &MemoryRingIOTest::BusyWriterTest);
		caller->addThread("WR #2", &MemoryRingIOTest::BusyWriterTest);
		caller->addThread("WR #3", &MemoryRingIOTest::BusyWriterTest);
		caller->addThread("RD", &MemoryRingIOTest::_DisableWriteOnFullBuffer);
		suite->addTest(caller);

		MemoryRingIOTest* endRead = new MemoryRingIOTest(sizeof(FULL_PAYLOAD));
		caller = new BThreadedTestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: RW threaded, writer set end reached on reader wait",
			endRead);
		caller->addThread("RD #1", &MemoryRingIOTest::BusyReaderTest);
		caller->addThread("RD #2", &MemoryRingIOTest::BusyReaderTest);
		caller->addThread("RD #3", &MemoryRingIOTest::BusyReaderTest);
		caller->addThread("WR", &MemoryRingIOTest::_DisableWriteOnEmptyBuffer);
		suite->addTest(caller);

		suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: RW single threaded with resizing",
			&MemoryRingIOTest::ReadWriteSingleTest, new MemoryRingIOTest(0)));
		suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: Attempt to truncate buffer",
			&MemoryRingIOTest::InvalidResizeTest, new MemoryRingIOTest(0)));
		suite->addTest(new CppUnit::TestCaller<MemoryRingIOTest>(
			"MemoryRingIOTest: Wait timeout",
			&MemoryRingIOTest::TimeoutTest, new MemoryRingIOTest(0)));

		return suite;
	}

private:
	BMemoryRingIO			fRing;
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MemoryRingIOTest, getTestSuiteName());
