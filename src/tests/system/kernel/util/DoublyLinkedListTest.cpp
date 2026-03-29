/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuiteAddon.h>
#include <TestUtils.h>
#include <cppunit/extensions/HelperMacros.h>

#include <util/DoublyLinkedList.h>


// Class used for testing without offset
class ItemWithout {
public:
	DoublyLinkedListLink<ItemWithout> fLink;
	int32 value;
};


// Class used for testing with offset
class ItemWith {
public:
	int32 value;
	DoublyLinkedListLink<ItemWith> fLink;
};


// Class used for testing without offset
class ItemVirtualWithout {
public:
	DoublyLinkedListLink<ItemVirtualWithout> fLink;
	int32 value;

	int32 Value() { return value; }
};


// Class used for testing with offset
class ItemVirtualWith {
public:
	int32 value;
	DoublyLinkedListLink<ItemVirtualWith> fLink;

	int32 Value() { return value; }
};


template<typename Item>
class DoublyLinkedListTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DoublyLinkedListTest);
	CPPUNIT_TEST(IsEmpty_ReturnsFalse);
	CPPUNIT_TEST(Count_ReturnsItemCount);
	CPPUNIT_TEST(Iterating_ItemsEqualAddedItems);
	CPPUNIT_TEST(RemoveHead_ReturnsFirstItem);
	CPPUNIT_TEST(HeadAndEverySecondItemRemoved_AddingFirstItem_ItemsAndCountAreCorrect);
	CPPUNIT_TEST_SUITE_END();

	typedef DoublyLinkedList<Item, DoublyLinkedListMemberGetLink<Item> > List;
	List* fList;
	static const int fValueCount = 10;
	Item fItems[fValueCount];

public:
	void setUp()
	{
		fList = new List;
		for (int i = 0; i < fValueCount; i++) {
			fItems[i].value = i;
			fList->Add(&fItems[i]);
		}
	}

	void tearDown()
	{
		delete fList;
	}

	void IsEmpty_ReturnsFalse()
	{
		CPPUNIT_ASSERT(!fList->IsEmpty());
	}

	void Count_ReturnsItemCount()
	{
		int count = 0;
		typename List::Iterator iterator = fList->GetIterator();
		while (iterator.Next() != NULL)
			count++;

		CPPUNIT_ASSERT(count == fValueCount);
	}

	void Iterating_ItemsEqualAddedItems()
	{
		int i = 0;
		typename List::Iterator iterator = fList->GetIterator();
		Item* item;
		while ((item = iterator.Next()) != NULL) {
			CPPUNIT_ASSERT(item->value == i);
			CPPUNIT_ASSERT(item == &fItems[i]);
			i++;
		}
	}

	void RemoveHead_ReturnsFirstItem()
	{
		Item* first = fList->RemoveHead();
		CPPUNIT_ASSERT(first->value == 0);
		CPPUNIT_ASSERT(first == &fItems[0]);
	}

	void HeadAndEverySecondItemRemoved_AddingFirstItem_ItemsAndCountAreCorrect()
	{
		fList->RemoveHead();
		typename List::Iterator iterator = fList->GetIterator();
		int i = 0;
		Item* item;
		while ((item = iterator.Next()) != NULL) {
			CPPUNIT_ASSERT(item->value == i + 1);

			if (i % 2)
				fList->Remove(item);
			i++;
		}

		fList->Add(&fItems[0]);

		int count = 0;
		iterator = fList->GetIterator();
		while (iterator.Next() != NULL)
			count++;

		CPPUNIT_ASSERT(count == (fValueCount / 2) + 1);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DoublyLinkedListTest<ItemWith>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DoublyLinkedListTest<ItemWithout>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DoublyLinkedListTest<ItemVirtualWith>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DoublyLinkedListTest<ItemVirtualWithout>, getTestSuiteName());
