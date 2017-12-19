/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ListTest.h"

#include <stdio.h>

#include <AutoDeleter.h>
#include <Json.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "List.h"

ListTest::ListTest()
{
}


ListTest::~ListTest()
{
}


static int32 CompareStrings(const BString& a, const BString& b)
{
	return a.Compare(b);
}


static int32 CompareWithContextString(const void* context, const BString& str)
{
	const char* contextString = static_cast<char*>(context);
	return -1 * str.Compare(contextString);
}


void
ListTest::TestBinarySearch()
{
	List<BString, false> list;
	BString tmp;

	for(char c = 'a'; c <= 'z'; c++) {
		tmp.SetToFormat("%c", c);
		list.AddOrdered(tmp, &CompareStrings);
	}

// ----------------------
	int32 aIndex = list.BinarySearch("a", &CompareWithContextString);
	int32 hIndex = list.BinarySearch("h", &CompareWithContextString);
	int32 uIndex = list.BinarySearch("u", &CompareWithContextString);
	int32 zIndex = list.BinarySearch("z", &CompareWithContextString);
	int32 ampersandIndex = list.BinarySearch("&", &CompareWithContextString);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(0, aIndex);
	CPPUNIT_ASSERT_EQUAL(7, hIndex);
	CPPUNIT_ASSERT_EQUAL(20, uIndex);
	CPPUNIT_ASSERT_EQUAL(25, zIndex);
	CPPUNIT_ASSERT_EQUAL(-1, ampersandIndex);
}


void
ListTest::TestAddOrdered()
{
	List<BString, false> list;

// ----------------------
	list.AddOrdered(BString("p"), &CompareStrings); //1
	list.AddOrdered(BString("o"), &CompareStrings);
	list.AddOrdered(BString("n"), &CompareStrings);
	list.AddOrdered(BString("s"), &CompareStrings);
	list.AddOrdered(BString("b"), &CompareStrings); //5
	list.AddOrdered(BString("y"), &CompareStrings);
	list.AddOrdered(BString("r"), &CompareStrings);
	list.AddOrdered(BString("d"), &CompareStrings);
	list.AddOrdered(BString("i"), &CompareStrings);
	list.AddOrdered(BString("k"), &CompareStrings); //10
	list.AddOrdered(BString("t"), &CompareStrings);
	list.AddOrdered(BString("e"), &CompareStrings);
	list.AddOrdered(BString("a"), &CompareStrings);
	list.AddOrdered(BString("u"), &CompareStrings);
	list.AddOrdered(BString("z"), &CompareStrings); // 15
	list.AddOrdered(BString("q"), &CompareStrings);
// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("expected count of package infos",
		16, list.CountItems());

	CPPUNIT_ASSERT_EQUAL(BString("a"), list.ItemAt(0));
	CPPUNIT_ASSERT_EQUAL(BString("b"), list.ItemAt(1));
	CPPUNIT_ASSERT_EQUAL(BString("d"), list.ItemAt(2));
	CPPUNIT_ASSERT_EQUAL(BString("e"), list.ItemAt(3));
	CPPUNIT_ASSERT_EQUAL(BString("i"), list.ItemAt(4));
	CPPUNIT_ASSERT_EQUAL(BString("k"), list.ItemAt(5));
	CPPUNIT_ASSERT_EQUAL(BString("n"), list.ItemAt(6));
	CPPUNIT_ASSERT_EQUAL(BString("o"), list.ItemAt(7));
	CPPUNIT_ASSERT_EQUAL(BString("p"), list.ItemAt(8));
	CPPUNIT_ASSERT_EQUAL(BString("q"), list.ItemAt(9));
	CPPUNIT_ASSERT_EQUAL(BString("r"), list.ItemAt(10));
	CPPUNIT_ASSERT_EQUAL(BString("s"), list.ItemAt(11));
	CPPUNIT_ASSERT_EQUAL(BString("t"), list.ItemAt(12));
	CPPUNIT_ASSERT_EQUAL(BString("u"), list.ItemAt(13));
	CPPUNIT_ASSERT_EQUAL(BString("y"), list.ItemAt(14));
	CPPUNIT_ASSERT_EQUAL(BString("z"), list.ItemAt(15));
}


/*static*/ void
ListTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"ListTest");

	suite.addTest(
		new CppUnit::TestCaller<ListTest>(
			"ListTest::TestAddOrdered",
			&ListTest::TestAddOrdered));

	suite.addTest(
		new CppUnit::TestCaller<ListTest>(
			"ListTest::TestBinarySearch",
			&ListTest::TestBinarySearch));

	parent.addTest("ListTest", &suite);
}