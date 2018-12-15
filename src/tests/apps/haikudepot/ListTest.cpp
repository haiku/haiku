/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
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
	const char* contextString = static_cast<const char*>(context);
	return -1 * str.Compare(contextString);
}


/*! This tests the insertion of various letters into the list and the subsequent
    search for those values later using a binary search.
*/

void
ListTest::TestBinarySearch()
{
	List<BString, false> list(&CompareStrings, &CompareWithContextString);
	BString tmp;

	for(char c = 'a'; c <= 'z'; c++) {
		tmp.SetToFormat("%c", c);
		list.Add(tmp);
	}

// ----------------------
	int32 aIndex = list.Search("a");
	int32 hIndex = list.Search("h");
	int32 uIndex = list.Search("u");
	int32 zIndex = list.Search("z");
	int32 ampersandIndex = list.Search("&");
// ----------------------

	CPPUNIT_ASSERT_EQUAL(0, aIndex);
	CPPUNIT_ASSERT_EQUAL(7, hIndex);
	CPPUNIT_ASSERT_EQUAL(20, uIndex);
	CPPUNIT_ASSERT_EQUAL(25, zIndex);
	CPPUNIT_ASSERT_EQUAL(-1, ampersandIndex);
}


/*! In this test, a number of letters are added into a list.  Later a check is
    made to ensure that the letters were added in order.
*/

void
ListTest::TestAddOrdered()
{
	List<BString, false> list(&CompareStrings, NULL);

// ----------------------

	list.Add(BString("p")); //1
	list.Add(BString("o"));
	list.Add(BString("n"));
	list.Add(BString("s"));
	list.Add(BString("b")); //5
	list.Add(BString("y"));
	list.Add(BString("r"));
	list.Add(BString("d"));
	list.Add(BString("i"));
	list.Add(BString("k")); //10
	list.Add(BString("t"));
	list.Add(BString("e"));
	list.Add(BString("a"));
	list.Add(BString("u"));
	list.Add(BString("z")); // 15
	list.Add(BString("q"));

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


/*! This test will add a number of letters to a list which has no ordering.  The
    letters should then appear in the list in the order of insertion.
*/

void
ListTest::TestAddUnordered()
{
	List<BString, false> list;

// ----------------------

	list.Add(BString("b"));
	list.Add(BString("e"));
	list.Add(BString("t"));
	list.Add(BString("h"));
	list.Add(BString("e"));
	list.Add(BString("l"));
	list.Add(BString("l"));
	list.Add(BString("s"));
	list.Add(BString("b"));
	list.Add(BString("e"));
	list.Add(BString("a"));
	list.Add(BString("c"));
	list.Add(BString("h"));

// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("expected count of package infos",
		13, list.CountItems());

	CPPUNIT_ASSERT_EQUAL(BString("b"), list.ItemAt(0));
	CPPUNIT_ASSERT_EQUAL(BString("e"), list.ItemAt(1));
	CPPUNIT_ASSERT_EQUAL(BString("t"), list.ItemAt(2));
	CPPUNIT_ASSERT_EQUAL(BString("h"), list.ItemAt(3));
	CPPUNIT_ASSERT_EQUAL(BString("e"), list.ItemAt(4));
	CPPUNIT_ASSERT_EQUAL(BString("l"), list.ItemAt(5));
	CPPUNIT_ASSERT_EQUAL(BString("l"), list.ItemAt(6));
	CPPUNIT_ASSERT_EQUAL(BString("s"), list.ItemAt(7));
	CPPUNIT_ASSERT_EQUAL(BString("b"), list.ItemAt(8));
	CPPUNIT_ASSERT_EQUAL(BString("e"), list.ItemAt(9));
	CPPUNIT_ASSERT_EQUAL(BString("a"), list.ItemAt(10));
	CPPUNIT_ASSERT_EQUAL(BString("c"), list.ItemAt(11));
	CPPUNIT_ASSERT_EQUAL(BString("h"), list.ItemAt(12));
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

	suite.addTest(
		new CppUnit::TestCaller<ListTest>(
			"ListTest::TestAddUnordered",
			&ListTest::TestAddUnordered));

	parent.addTest("ListTest", &suite);
}