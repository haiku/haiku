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

	return suite;
}

// Class used for testing default User strategy
class Link {
public:
	SinglyLinkedListLink<Link> next;
	long data;
	
	SinglyLinkedListLink<Link>* GetSinglyLinkedListLink() {
		return &next;
	}

	bool operator==(const Link &ref) {
		return data == ref.data;
	}
};

// Class used for testing custom User strategy
class MyLink {
public:
	SinglyLinkedListLink<MyLink> mynext;
	long data;

	bool operator==(const MyLink &ref) {
		return data == ref.data;
	}
};

//! Tests the given list
template <class List, class Element>
void
SinglyLinkedListTest::TestList(List &list, Element *values, int valueCount)
{
	list.MakeEmpty();

	// PushFront
	for (int i = 0; i < valueCount; i++) {
		NextSubTest();
		CHK(list.Size() == i);
		list.Add(&values[i]);
		CHK(list.Size() == i+1);
	}
	
	// Prefix increment
	int preIndex = valueCount-1;
	for (typename List::Iterator iterator = list.GetIterator();
			iterator.HasNext(); --preIndex) {
		NextSubTest();

 		Element* element = iterator.Next();
		CHK(*element == values[preIndex]);
	}
	CHK(preIndex == -1);
	list.MakeEmpty();	
}

//! Test using the User strategy with the default NextMember.
void
SinglyLinkedListTest::UserDefaultTest() {
	SinglyLinkedList<Link> list;
	const int valueCount = 10;
	Link values[valueCount];
	for (int i = 0; i < valueCount; i++) {
		values[i].data = i;
		if (i % 2)
			values[i].next.next = NULL;	// Leave some next pointers invalid just for fun
	}
	
	TestList(list, values, valueCount);
}

//! Test using the User strategy with a custom NextMember.
void
SinglyLinkedListTest::UserCustomTest() {
	SinglyLinkedList<MyLink, SinglyLinkedListMemberGetLink<MyLink, &MyLink::mynext> > list;
	const int valueCount = 10;
	MyLink values[valueCount];
	for (int i = 0; i < valueCount; i++) {
		values[i].data = i*2;
		if (!(i % 2))
			values[i].mynext.next = NULL;	// Leave some next pointers invalid just for fun
	}
	
	TestList(list, values, valueCount);
}
