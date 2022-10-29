/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "ExclusiveBorrowTest.h"

#include <atomic>
#include <tuple>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <ExclusiveBorrow.h>

using BPrivate::Network::BBorrow;
using BPrivate::Network::BBorrowError;
using BPrivate::Network::BExclusiveBorrow;
using BPrivate::Network::make_exclusive_borrow;


class DeleteTestHelper
{
public:
	DeleteTestHelper(std::atomic<bool>& deleted)
		:
		fDeleted(deleted)
	{
	}

	~DeleteTestHelper() { fDeleted.store(true); }

private:
	std::atomic<bool>& fDeleted;
};


class Base
{
public:
	Base() {}


	virtual ~Base() {}


	virtual bool IsDerived() { return false; }
};


class Derived : public Base
{
public:
	Derived() {}


	virtual ~Derived() {}


	virtual bool IsDerived() override { return true; }
};


ExclusiveBorrowTest::ExclusiveBorrowTest()
{
}


void
ExclusiveBorrowTest::ObjectDeleteTest()
{
	// Case 1: object never gets borrowed and goes out of scope
	std::atomic<bool> deleted = false;
	{
		auto object = make_exclusive_borrow<DeleteTestHelper>(deleted);
	}
	CPPUNIT_ASSERT_EQUAL_MESSAGE("(1) Expected object to be deleted", true, deleted.load());

	// Case 2: object gets borrowed, returned and then goes out of scope
	deleted.store(false);
	{
		auto object = make_exclusive_borrow<DeleteTestHelper>(deleted);
		{
			auto borrow = BBorrow<DeleteTestHelper>(object);
		}
		CPPUNIT_ASSERT_EQUAL_MESSAGE("(2) Object should not be deleted", false, deleted.load());
	}
	CPPUNIT_ASSERT_EQUAL_MESSAGE("(2) Expected object to be deleted", true, deleted.load());

	// Case 3: object gets borrowed, forfeited and then borrow goes out of scope
	deleted.store(false);
	{
		auto borrow = BBorrow<DeleteTestHelper>(nullptr);
		{
			auto object = make_exclusive_borrow<DeleteTestHelper>(deleted);
			borrow = BBorrow<DeleteTestHelper>(object);
		}
		CPPUNIT_ASSERT_EQUAL_MESSAGE("(3) Object should not be deleted", false, deleted.load());
	}
	CPPUNIT_ASSERT_EQUAL_MESSAGE("(3) Expected object to be deleted", true, deleted.load());
}


void
ExclusiveBorrowTest::OwnershipTest()
{
	auto ownedObject = make_exclusive_borrow<int>(1);
	CPPUNIT_ASSERT(*ownedObject == 1);

	auto borrow = BBorrow<int>(ownedObject);
	try {
		std::ignore = *ownedObject == 1;
		CPPUNIT_FAIL("Unexpected access to the owned object while borrowed");
	} catch (const BBorrowError& e) {
		// expected
	}

	try {
		std::ignore = *borrow == 1;
		// should succeed
	} catch (const BBorrowError& e) {
		CPPUNIT_FAIL("Unexpected error accessing the borrowed object");
	}

	try {
		auto borrowAgain = BBorrow<int>(ownedObject);
		CPPUNIT_FAIL("Unexpectedly able to borrow the owned object again");
	} catch (const BBorrowError& e) {
		// expected
	}

	try {
		borrow = BBorrow<int>(nullptr);
		std::ignore = *borrow == 1;
		CPPUNIT_FAIL("Unexpected access to an empty borrowed object");
	} catch (const BBorrowError& e) {
		// expected
	}

	try {
		std::ignore = *ownedObject == 1;
	} catch (const BBorrowError& e) {
		CPPUNIT_FAIL("Unexpected error accessing the owned object");
	}
}


void
ExclusiveBorrowTest::PolymorphismTest()
{
	auto owned = make_exclusive_borrow<Derived>();
	{
		auto borrowDerived = BBorrow<Derived>(owned);
		CPPUNIT_ASSERT_EQUAL(true, borrowDerived->IsDerived());
	}
	{
		auto borrowBase = BBorrow<Base>(owned);
		CPPUNIT_ASSERT_EQUAL(true, borrowBase->IsDerived());
	}
}


void
ExclusiveBorrowTest::ReleaseTest()
{
	auto ownedObject = make_exclusive_borrow<int>(1);
	auto ownedPointer = std::addressof(*ownedObject);
	try {
		auto borrow = BBorrow<int>(ownedObject);
		auto invalidClaimedPointer = ownedObject.Release();
		CPPUNIT_FAIL("Unexpectedly able to release a borrowed pointer");
	} catch (const BBorrowError&) {
		// expected to fail
	}

	auto validClaimedPointer = ownedObject.Release();
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected released pointer to point to the same object",
		validClaimedPointer.get(), ownedPointer);
}


/* static */ void
ExclusiveBorrowTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("ExclusiveBorrowTest");

	suite.addTest(new CppUnit::TestCaller<ExclusiveBorrowTest>(
		"ExclusiveBorrowTest::ObjectDeleteTest", &ExclusiveBorrowTest::ObjectDeleteTest));
	suite.addTest(new CppUnit::TestCaller<ExclusiveBorrowTest>(
		"ExclusiveBorrowTest::OwnershipTest", &ExclusiveBorrowTest::OwnershipTest));
	suite.addTest(new CppUnit::TestCaller<ExclusiveBorrowTest>(
		"ExclusiveBorrowTest::PolymorphismTest", &ExclusiveBorrowTest::PolymorphismTest));
	suite.addTest(new CppUnit::TestCaller<ExclusiveBorrowTest>(
		"ExclusiveBorrowTest::ReleaseTest", &ExclusiveBorrowTest::ReleaseTest));

	parent.addTest("ExclusiveBorrowTest", &suite);
}
