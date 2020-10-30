/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LRUCacheTest.h"

#include <stdio.h>

#include <HashString.h>
#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "LRUCache.h"


LRUCacheTest::LRUCacheTest()
{
}


LRUCacheTest::~LRUCacheTest()
{
}


/*! This tests the insertion of various letters into the map and the subsequent
    search for those values later using a binary search.
*/

void
LRUCacheTest::TestAddWithOverflow()
{
	LRUCache<HashString, BString> map(5);

	BString tmpKey;
	BString tmpValue;

// ----------------------
	for(char c = 'a'; c <= 'z'; c++) {
		tmpKey.SetToFormat("%c", c);
		tmpValue.SetToFormat("%c%c", c, c);
		map.Put(HashString(tmpKey), tmpValue);
	}
// ----------------------

	CPPUNIT_ASSERT_EQUAL(5, map.Size());
	// oldest entries have been removed.
	CPPUNIT_ASSERT_EQUAL(BString(""), map.Get(HashString("a")));
	CPPUNIT_ASSERT_EQUAL(BString(""), map.Get(HashString("u")));
	// latter entries have been removed.
	CPPUNIT_ASSERT_EQUAL(BString("zz"), map.Get(HashString("z")));
	CPPUNIT_ASSERT_EQUAL(BString("yy"), map.Get(HashString("y")));
	CPPUNIT_ASSERT_EQUAL(BString("xx"), map.Get(HashString("x")));
	CPPUNIT_ASSERT_EQUAL(BString("ww"), map.Get(HashString("w")));
	CPPUNIT_ASSERT_EQUAL(BString("vv"), map.Get(HashString("v")));
}


/*! This tests the insertion of various letters into the list, but during the
	inserts, there are some get operations which will effect which are
	considered to be the oldest entries.
*/

void
LRUCacheTest::TestAddWithOverflowWithGets()
{
	LRUCache<HashString, BString> map(3);

// ----------------------
	map.Put(HashString("Red"), "Rot");
	map.Put(HashString("Yellow"), "Gelb");
	map.Get(HashString("Red"));
	map.Put(HashString("Green"), "Gruen");
	map.Put(HashString("Purple"), "Lila");
// ----------------------

	CPPUNIT_ASSERT_EQUAL(3, map.Size());
	CPPUNIT_ASSERT_EQUAL(BString(""), map.Get(HashString("Yellow")));
	CPPUNIT_ASSERT_EQUAL(BString("Rot"), map.Get(HashString("Red")));
	CPPUNIT_ASSERT_EQUAL(BString("Gruen"), map.Get(HashString("Green")));
	CPPUNIT_ASSERT_EQUAL(BString("Lila"), map.Get(HashString("Purple")));
}


void
LRUCacheTest::TestRemove()
{
	LRUCache<HashString, BString> map(3);

	// control value
	map.Put(HashString("Town"), "Tirau");
	map.Put(HashString("Lake"), "Taupo");

// ----------------------
	BString resultOcean = map.Remove(HashString("Ocean"));
	BString resultLake = map.Remove(HashString("Lake"));
// ----------------------

	CPPUNIT_ASSERT_EQUAL(1, map.Size());
	CPPUNIT_ASSERT_EQUAL(BString(""), resultOcean);
	CPPUNIT_ASSERT_EQUAL(BString("Taupo"), resultLake);
	CPPUNIT_ASSERT_EQUAL(BString("Tirau"), map.Get(HashString("Town")));
	CPPUNIT_ASSERT_EQUAL(BString(""), map.Get(HashString("Lake")));
	CPPUNIT_ASSERT_EQUAL(BString(""), map.Get(HashString("Ocean")));
}


/*static*/ void
LRUCacheTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"LRUCacheTest");

	suite.addTest(
		new CppUnit::TestCaller<LRUCacheTest>(
			"LRUCacheTest::TestAddWithOverflow",
			&LRUCacheTest::TestAddWithOverflow));
	suite.addTest(
		new CppUnit::TestCaller<LRUCacheTest>(
			"LRUCacheTest::TestAddWithOverflowWithGets",
			&LRUCacheTest::TestAddWithOverflowWithGets));
	suite.addTest(
		new CppUnit::TestCaller<LRUCacheTest>(
			"LRUCacheTest::TestRemove",
			&LRUCacheTest::TestRemove));

	parent.addTest("LRUCacheTest", &suite);
}
