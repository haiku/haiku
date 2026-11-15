/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Looper.h>
#include <Messenger.h>

#include "common.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class LooperTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(LooperTest);
	CPPUNIT_TEST(ValidTid_LooperForThread_ReturnsLooper);
	CPPUNIT_TEST(InvalidTid_LooperForThread_ReturnsNull);
	CPPUNIT_TEST(CalledTwice_Run_Debugger);
	CPPUNIT_TEST(CheckThreadId_Run_Matches);
	CPPUNIT_TEST(DeleteRunning_Run_Debugger);
	CPPUNIT_TEST(Unlocked_Quit_Error);
	CPPUNIT_TEST(ViaMessage_Quit_Succeeds);
	CPPUNIT_TEST(Looper_Sizeof_MatchR5);
	CPPUNIT_TEST(EmptyLooper_Perform_ReturnsNameNotFound);
	CPPUNIT_TEST_SUITE_END();

public:
	void ValidTid_LooperForThread_ReturnsLooper()
	{
		BLooper* Looper = new BLooper;
		thread_id tid = Looper->Run();
		CPPUNIT_ASSERT(Looper == BLooper::LooperForThread(tid));
		Looper->Lock();
		Looper->Quit();
	}

	void InvalidTid_LooperForThread_ReturnsNull()
	{
		CPPUNIT_ASSERT(BLooper::LooperForThread(find_thread(NULL)) == NULL);
	}

	/**
		Run()
		@case		Attempt to call Run() twice
		@results	debugger message "can't call BLooper::Run twice!"
	 */
	void CalledTwice_Run_Debugger()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		Looper.Run();
		Looper.Run();
		Looper.Quit();
	}

	/**
		Run()
		@case		Check thread_id of Looper
		@results	Run() and Thread() return the same thread_id
	 */
	void CheckThreadId_Run_Matches()
	{
		BLooper* Looper = new BLooper;
		thread_id tid = Looper->Run();
		CPPUNIT_ASSERT(tid == Looper->Thread());
		Looper->Lock();
		Looper->Quit();
	}

	/**
		Run()
		@case		Delete looper after calling Run()
		@results	Debugger message "You can't call delete on a BLooper object
					once it is running."
	 */
	void DeleteRunning_Run_Debugger()
	{
		DEBUGGER_ESCAPE;

		BLooper* Looper = new BLooper;
		Looper->Run();
		delete Looper;
	}

	/**
		Quit()
		@case	Looper is unlocked
		@result	Prints message "ERROR - you must Lock a looper before calling
				Quit(), team=%ld, looper=%s\n"
	 */
	void Unlocked_Quit_Error()
	{
		BLooper* looper = new BLooper;
		looper->Unlock();
		looper->Quit();
	}

	void ViaMessage_Quit_Succeeds()
	{
		BLooper* looper = new BLooper;
		looper->Run();

		BMessage reply;
		BMessenger(looper).SendMessage(B_QUIT_REQUESTED, &reply);
	}

	void Looper_Sizeof_MatchR5()
	{
#ifdef B_HAIKU_32_BIT
		const size_t R5BLooperSize = 172;
		CPPUNIT_ASSERT(sizeof (BLooper) == R5BLooperSize);
#else
		CPPUNIT_ASSERT(true);
#endif
	}

	void EmptyLooper_Perform_ReturnsNameNotFound()
	{
		BLooper Looper;
		CPPUNIT_ASSERT_EQUAL(B_NAME_NOT_FOUND, Looper.Perform(1, NULL));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LooperTest, getTestSuiteName());
