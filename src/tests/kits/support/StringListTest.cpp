/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <String.h>
#include <StringList.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


static int
CompareWithData(const char* a, const char* b, void* context)
{
	return strcmp(a, b);
}


static int
CompareWithDataBString(const BString& a, const BString& b, void* context)
{
	return a.Compare(b);
}




class StringListTest : public CppUnit::TestFixture {
public:
	CPPUNIT_TEST_SUITE(StringListTest);
	CPPUNIT_TEST(SortItems_IsSorted);
	CPPUNIT_TEST_SUITE_END();

	void SortItems_IsSorted();

private:
	void Initialize(BStringList& list, int size);
	bool Equals(const BStringList& list1, const BStringList& list2);
	bool IsSorted(const BStringList& list);
};


void
StringListTest::Initialize(BStringList& list, int size)
{
	const char* fruits[] = {"Apple", "Orange", "Banana", "Grapes", "Cherry"};
	for (int32 i = 0; i < size; i++)
		list.Add(BString(fruits[i % 5]));
}


bool
StringListTest::Equals(const BStringList& list1, const BStringList& list2)
{
	const int32 n = list1.CountStrings();
	if (n != list2.CountStrings())
		return false;

	for (int32 i = 0; i < n; i++) {
		BString item1 = list1.StringAt(i);
		BString item2 = list2.StringAt(i);
		if (item1 != item2)
			return false;
	}

	return true;
}


bool
StringListTest::IsSorted(const BStringList& list)
{
	BString previtem = list.StringAt(0);
	for (int32 i = 1; i < list.CountStrings(); i++) {
		BString item = list.StringAt(i);
		if (item.Compare(previtem) < 0)
			return false;
		previtem = item;
	}
	return true;
}


void
StringListTest::SortItems_IsSorted()
{
	for (int i = 10; i <= 20; i++) {
		BStringList list;
		Initialize(list, i);

		BStringList clone(list);
		CPPUNIT_ASSERT(Equals(list, clone));

		list.Sort(CompareWithData, NULL);
		CPPUNIT_ASSERT(IsSorted(list));

		clone.Sort(CompareWithDataBString, NULL);
		CPPUNIT_ASSERT(IsSorted(clone));
		CPPUNIT_ASSERT(Equals(list, clone));
	}
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringListTest, getTestSuiteName());
