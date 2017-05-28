/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "CollatorTest.h"

#include <Collator.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


CollatorTest::CollatorTest()
{
}


CollatorTest::~CollatorTest()
{
}


void
CollatorTest::TestSortKeys()
{
	struct Test {
		char* first;
		char* second;
		int sign[3];
	};

	BCollator collator;
	BLocaleRoster::Default()->GetDefaultLocale()->GetCollator(&collator);
	const Test tests[] = {
		{"gehen", "géhen", {0, -1, -1}},
		{"aus", "äUß", {-1, -1, -1}},
		{"auss", "äUß", {0, -1, -1}},
		{"WO", "wÖ", {0, -1, -1}},
		{"SO", "so", {0, 0, 1}},
		{"açñ", "acn", {0, 1, 1}},
		{NULL, NULL, {0, 0, 0}}
	};
	
	for (int32 i = 0; tests[i].first != NULL; i++) {
		NextSubTest();

		for (int32 strength = B_COLLATE_PRIMARY; strength < 4; strength++) {
			BString a, b;
			collator.SetStrength(strength);
			collator.GetSortKey(tests[i].first, &a);
			collator.GetSortKey(tests[i].second, &b);

			int difference = collator.Compare(tests[i].first, tests[i].second);
			CPPUNIT_ASSERT_EQUAL(tests[i].sign[strength - 1], difference);
			int keydiff = strcmp(a.String(), b.String());
			// Check that the keys compare the same as the strings. Either both
			// are 0, or both have the same sign.
			if (difference == 0)
				CPPUNIT_ASSERT_EQUAL(0, keydiff);
			else
				CPPUNIT_ASSERT(keydiff * difference > 0);
		}
	}
}


/*static*/ void
CollatorTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("CollatorTest");

	suite.addTest(new CppUnit::TestCaller<CollatorTest>(
		"CollatorTest::TestSortKeys", &CollatorTest::TestSortKeys));

	parent.addTest("CollatorTest", &suite);
}
