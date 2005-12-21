#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <TestUtils.h>

#include "SinglyLinkedListTest.h"
#include "SinglyLinkedList.h"

SinglyLinkedListTest::SinglyLinkedListTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
SinglyLinkedListTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("SLL");

	suite->addTest(new CppUnit::TestCaller<SinglyLinkedListTest>("SinglyLinkedList::User Strategy Test (default next parameter)", &SinglyLinkedListTest::UserDefaultTest));
	suite->addTest(new CppUnit::TestCaller<SinglyLinkedListTest>("SinglyLinkedList::User Strategy Test (custom next parameter)", &SinglyLinkedListTest::UserCustomTest));
	suite->addTest(new CppUnit::TestCaller<SinglyLinkedListTest>("SinglyLinkedList::Auto Strategy Test (MallocFreeAllocator)", &SinglyLinkedListTest::AutoTest));

	return suite;
}

// Class used for testing default User strategy
class Link {
public:
	Link* next;
	long data;
	
	bool operator==(const Link &ref) {
		return data == ref.data;
	}
};

// Class used for testing custom User strategy
class MyLink {
public:
	MyLink* mynext;
	long data;

	bool operator==(const MyLink &ref) {
		return data == ref.data;
	}
};

using Strategy::SinglyLinkedList::User;
using Strategy::SinglyLinkedList::Auto;

//! Tests the given list
template <class List>
void
SinglyLinkedListTest::TestList(List &list, typename List::ValueType *values, int valueCount)
{
	list.MakeEmpty();

	// PushFront
	for (int i = 0; i < valueCount; i++) {
		NextSubTest();
		CHK(list.Count() == i);
		CHK(list.PushFront(values[i]) == B_OK);
		CHK(list.Count() == i+1);
	}
	
{
	// Prefix increment
	int preIndex = valueCount-1;
	typename List::Iterator iterator;
	for (iterator = list.Begin(); iterator != list.End(); --preIndex) {
		NextSubTest();
// 		printf("(%p, %ld) %s (%p, %ld)\n", iterator->next, iterator->data, ((*iterator == values[preIndex]) ? "==" : "!="), values[preIndex].next, values[preIndex].data);
		CHK(*iterator == values[preIndex]);
		typename List::Iterator copy = iterator;
		CHK(copy == iterator);
		CHK(copy != ++iterator);
	}
	CHK(preIndex == -1);
}	
	list.MakeEmpty();	

	// PushBack
	for (int i = 0; i < valueCount; i++) {
		NextSubTest();
		CHK(list.Count() == i);
		CHK(list.PushBack(values[i]) == B_OK);
		CHK(list.Count() == i+1);
	}
	
	// Postfix increment
	int postIndex = 0;
	for (typename List::Iterator iterator = list.Begin(); iterator != list.End(); ++postIndex) {
		NextSubTest();
// 		printf("(%p, %ld) %s (%p, %ld)\n", iterator->next, iterator->data, ((*iterator == values[postIndex]) ? "==" : "!="), values[postIndex].next, values[postIndex].data);
		CHK(*iterator == values[postIndex]);
		typename List::Iterator copy = iterator;
		CHK(copy == iterator);
		CHK(copy == iterator++);
	}
	CHK(postIndex == valueCount);
	
}

//! Test using the User strategy with the default NextMember.
void
SinglyLinkedListTest::UserDefaultTest() {
	SinglyLinkedList<Link, User<Link> > list;
	const int valueCount = 10;
	Link values[valueCount];
	for (int i = 0; i < valueCount; i++) {
		values[i].data = i;
		if (i % 2)
			values[i].next = NULL;	// Leave some next pointers invalid just for fun
	}
	
	TestList(list, values, valueCount);
}

//! Test using the User strategy with a custom NextMember.
void
SinglyLinkedListTest::UserCustomTest() {
	SinglyLinkedList<MyLink, User<MyLink, &MyLink::mynext> > list;
	const int valueCount = 10;
	MyLink values[valueCount];
	for (int i = 0; i < valueCount; i++) {
		values[i].data = i*2;
		if (!(i % 2))
			values[i].mynext = NULL;	// Leave some next pointers invalid just for fun
	}
	
	TestList(list, values, valueCount);
}

//! Test using the Auto strategy.
void
SinglyLinkedListTest::AutoTest() {
	SinglyLinkedList<long> list;
	const int valueCount = 10;
	long values[valueCount];
	for (int i = 0; i < valueCount; i++) {
		values[i] = i*3;
	}
	
	TestList(list, values, valueCount);
}
