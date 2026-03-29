/*
 * Copyright 2003-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "SinglyLinkedList.h"


// Class used for testing default User strategy
class Link {
public:
	SinglyLinkedListLink<Link> next;
	long data;

	SinglyLinkedListLink<Link>* GetSinglyLinkedListLink() { return &next; }

	bool operator==(const Link& ref) { return data == ref.data; }
};


// Class used for testing custom User strategy
class MyLink {
public:
	SinglyLinkedListLink<MyLink> next;
	long data;

	bool operator==(const MyLink& ref) { return data == ref.data; }
};


template<typename Link, typename List>
class SinglyLinkedListTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SinglyLinkedListTest);
	CPPUNIT_TEST(TwoValues_MakeEmpty_ListIsEmpty);
	CPPUNIT_TEST(EmptyList_AddingValues_CountIncreases);
	CPPUNIT_TEST(TenValues_AccessingWithIterator_IteratesThroughAllValues);
	CPPUNIT_TEST_SUITE_END();

	List* fList;
	static const int fValueCount = 10;
	Link fValues[fValueCount];

public:
	void setUp()
	{
		fList = new List;
		for (int i = 0; i < fValueCount; i++) {
			fValues[i].data = i * 2;
			if (!(i % 2))
				fValues[i].next.next = NULL; // Leave some next pointers invalid just for fun
		}
	}

	void tearDown()
	{
		delete fList;
	}

	void TwoValues_MakeEmpty_ListIsEmpty()
	{
		Link value1, value2;
		value1.data = 2;
		value2.data = 5;
		fList->Add(&value1);
		fList->Add(&value2);
		CPPUNIT_ASSERT(fList->Count() == 2);

		fList->MakeEmpty();
		CPPUNIT_ASSERT(fList->Count() == 0);
	}

	void EmptyList_AddingValues_CountIncreases() {
		for (int i = 0; i < fValueCount; i++) {
			CPPUNIT_ASSERT(fList->Count() == i);
			fList->Add(&fValues[i]);
			CPPUNIT_ASSERT(fList->Count() == i + 1);
		}
	}

	void TenValues_AccessingWithIterator_IteratesThroughAllValues()
	{
		for (int i = 0; i < fValueCount; i++) {
			fList->Add(&fValues[i]);
		}

		// Prefix increment
		int preIndex = fValueCount - 1;
		for (typename List::ConstIterator iterator = fList->GetIterator(); iterator.HasNext();
				--preIndex) {
			Link* element = iterator.Next();
			CPPUNIT_ASSERT(*element == fValues[preIndex]);
		}
		CPPUNIT_ASSERT(preIndex == -1);
	}
};


typedef SinglyLinkedListTest<Link, SinglyLinkedList<Link> > SinglyLinkedListDefaultTest;
typedef SinglyLinkedListTest<MyLink,
	SinglyLinkedList<MyLink, SinglyLinkedListMemberGetLink<MyLink, &MyLink::next> > >
	SinglyLinkedListCustomTest;
//! Test using the User strategy with the default NextMember.
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SinglyLinkedListDefaultTest, getTestSuiteName());
//! Test using the User strategy with a custom NextMember.
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SinglyLinkedListCustomTest, getTestSuiteName());
