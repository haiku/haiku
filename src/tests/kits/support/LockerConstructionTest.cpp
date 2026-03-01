/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		tylerdauwalder
 */


#include <Locker.h>
#include <OS.h>

#include <TestSuiteAddon.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string.h>


class LockerConstructionTest : public CppUnit::TestFixture {
public:
	CPPUNIT_TEST_SUITE(LockerConstructionTest);
	CPPUNIT_TEST(Constructor_Default_BenaphoreWithDefaultName);
	CPPUNIT_TEST(Constructor_WithName_BenaphoreWithSpecifiedName);
	CPPUNIT_TEST(Constructor_WithBenaphoreFalse_SemaphoreWithDefaultName);
	CPPUNIT_TEST(Constructor_WithBenaphoreTrue_BenaphoreWithDefaultName);
	CPPUNIT_TEST(Constructor_WithNameAndBenaphoreFalse_SemaphoreWithSpecifiedName);
	CPPUNIT_TEST(Constructor_WithNameAndBenaphoreTrue_BenaphoreWithSpecifiedName);
	CPPUNIT_TEST_SUITE_END();

private:
	bool NameMatches(const char* name, BLocker* lockerArg)
	{
		sem_info theSemInfo;
		CPPUNIT_ASSERT(get_sem_info(lockerArg->Sem(), &theSemInfo) == B_OK);
		return strcmp(name, theSemInfo.name) == 0;
	}

	bool IsBenaphore(BLocker* lockerArg)
	{
		int32 semCount;
		CPPUNIT_ASSERT(get_sem_count(lockerArg->Sem(), &semCount) == B_OK);
		switch (semCount) {
			case 0:
				return true;
			case 1:
				return false;
			default:
				// Unexpected semaphore count
				CPPUNIT_ASSERT(false);
				break;
		}
		return false;
	}

public:
	void Constructor_Default_BenaphoreWithDefaultName()
	{
		BLocker locker;
		CPPUNIT_ASSERT(NameMatches("some BLocker", &locker));
		CPPUNIT_ASSERT(IsBenaphore(&locker));
	}

	void Constructor_WithName_BenaphoreWithSpecifiedName()
	{
		BLocker locker("test string");
		CPPUNIT_ASSERT(NameMatches("test string", &locker));
		CPPUNIT_ASSERT(IsBenaphore(&locker));
	}

	void Constructor_WithBenaphoreFalse_SemaphoreWithDefaultName()
	{
		BLocker locker(false);
		CPPUNIT_ASSERT(NameMatches("some BLocker", &locker));
		CPPUNIT_ASSERT(!IsBenaphore(&locker));
	}

	void Constructor_WithBenaphoreTrue_BenaphoreWithDefaultName()
	{
		BLocker locker(true);
		CPPUNIT_ASSERT(NameMatches("some BLocker", &locker));
		CPPUNIT_ASSERT(IsBenaphore(&locker));
	}

	void Constructor_WithNameAndBenaphoreFalse_SemaphoreWithSpecifiedName()
	{
		BLocker locker("test string", false);
		CPPUNIT_ASSERT(NameMatches("test string", &locker));
		CPPUNIT_ASSERT(!IsBenaphore(&locker));
	}

	void Constructor_WithNameAndBenaphoreTrue_BenaphoreWithSpecifiedName()
	{
		BLocker locker("test string", true);
		CPPUNIT_ASSERT(NameMatches("test string", &locker));
		CPPUNIT_ASSERT(IsBenaphore(&locker));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LockerConstructionTest, getTestSuiteName());
