/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "DoublyLinkedListTest.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestUtils.h>

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
		virtual int32 Value();

		DoublyLinkedListLink<ItemVirtualWithout> fLink;
		int32 value;
};

// Class used for testing with offset
class ItemVirtualWith {
	public:
		virtual int32 Value();

		int32 value;
		DoublyLinkedListLink<ItemVirtualWith> fLink;
};


int32 
ItemVirtualWithout::Value()
{
	return value;
}


int32 
ItemVirtualWith::Value()
{
	return value;
}


//	#pragma mark -


DoublyLinkedListTest::DoublyLinkedListTest(std::string name)
	: BTestCase(name)
{
}


CppUnit::Test*
DoublyLinkedListTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("DLL");

	suite->addTest(new CppUnit::TestCaller<DoublyLinkedListTest>("DoublyLinkedList::no offset", &DoublyLinkedListTest::WithoutOffsetTest));
	suite->addTest(new CppUnit::TestCaller<DoublyLinkedListTest>("DoublyLinkedList::with offset", &DoublyLinkedListTest::WithOffsetTest));
	suite->addTest(new CppUnit::TestCaller<DoublyLinkedListTest>("DoublyLinkedList::virtual no offset", &DoublyLinkedListTest::VirtualWithoutOffsetTest));
	suite->addTest(new CppUnit::TestCaller<DoublyLinkedListTest>("DoublyLinkedList::virtual with offset", &DoublyLinkedListTest::VirtualWithOffsetTest));

	return suite;
}


//! Tests the given list

template <typename Item>
void
DoublyLinkedListTest::TestList()
{
	DoublyLinkedList<Item, DoublyLinkedListMemberGetLink<Item> > list;
	int valueCount = 10;
	Item items[valueCount];

	// initialize

	for (int i = 0; i < valueCount; i++) {
		items[i].value = i;
		list.Add(&items[i]);
	}

	// list must not be empty

	CHK(!list.IsEmpty());

	// count items in list

	int count = 0;
	typename DoublyLinkedList<Item,
			DoublyLinkedListMemberGetLink<Item> >::Iterator
		iterator = list.GetIterator();
	while (iterator.Next() != NULL)
		count++;

	CHK(count == valueCount);

	// test for equality

	iterator = list.GetIterator();

	int i = 0;
	Item *item;
	while ((item = iterator.Next()) != NULL) {
		CHK(item->value == i);
		CHK(item == &items[i]);
		i++;
	}

	// remove first

	Item *first = list.RemoveHead();
	CHK(first->value == 0);
	CHK(first == &items[0]);
	
	// remove every second
	
	iterator = list.GetIterator();
	i = 0;
	while ((item = iterator.Next()) != NULL) {
		CHK(item->value == i + 1);

		if (i % 2)
			list.Remove(item);
		i++;
	}
	
	// re-add first
	
	list.Add(first);

	// count again

	count = 0;
	iterator = list.GetIterator();
	while (iterator.Next() != NULL)
		count++;

	CHK(count == (valueCount / 2) + 1);
}


//! Test using no offset, no virtual

void
DoublyLinkedListTest::WithoutOffsetTest() {
	TestList<ItemWithout>();
}


//! Test using offset, no virtual

void
DoublyLinkedListTest::WithOffsetTest() {
	TestList<ItemWith>();
}


//! Test using no offset, virtual

void
DoublyLinkedListTest::VirtualWithoutOffsetTest() {
	TestList<ItemVirtualWithout>();
}


//! Test using offset, virtual

void
DoublyLinkedListTest::VirtualWithOffsetTest() {
	TestList<ItemVirtualWith>();
}

