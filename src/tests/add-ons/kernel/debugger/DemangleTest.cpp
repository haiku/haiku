/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DemangleTest.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "Demangler.h"


DemangleTest::DemangleTest()
{
}


DemangleTest::~DemangleTest()
{
}


#define TEST(expect, input) \
	NextSubTest(); \
	CPPUNIT_ASSERT_EQUAL(BString(expect), Demangler::Demangle(input))
void
DemangleTest::RunGCC2Tests()
{
	// Long and complex things
	TEST("BPrivate::IconCache::SyncDraw(BPrivate::Model*, BView*, BPoint, BPrivate::IconDrawMode, icon_size, void*, void*)",
		"SyncDraw__Q28BPrivate9IconCachePQ28BPrivate5ModelP5BViewG6BPointQ28BPrivate12IconDrawMode9icon_sizePFP5BViewG6BPointP7BBitmapPv_vPv");
	TEST("BPrivate::BContainerWindow::UpdateMenu(BMenu*, BPrivate::BContainerWindow::UpdateMenuContext)",
		"UpdateMenu__Q28BPrivate16BContainerWindowP5BMenuQ38BPrivate16BContainerWindow17UpdateMenuContext");
	TEST("icu_57::BreakIterator::registerInstance(icu_57::BreakIterator*, icu_57::Locale&, UBreakIteratorType, UErrorCode&)",
		"registerInstance__Q26icu_5713BreakIteratorPQ26icu_5713BreakIteratorRCQ26icu_576Locale18UBreakIteratorTypeR10UErrorCode");

	// Previously caused crashes
	TEST("SetTo__Q282_GLOBAL_", "_GLOBAL_::SetTo()");
}


void
DemangleTest::RunGCC3PTests()
{
	// Long and complex things
	TEST("BPrivate::IconCache::SyncDraw(BPrivate::Model*, BView*, BPoint, BPrivate::IconDrawMode, icon_size, void (*)(BView*, BPoint, BBitmap*, void*), void*)",
		"_ZN8BPrivate9IconCache8SyncDrawEPNS_5ModelEP5BView6BPointNS_12IconDrawModeE9icon_sizePFvS4_S5_P7BBitmapPvESA_");
	TEST("BPrivate::BContainerWindow::UpdateMenu(BMenu*, BPrivate::BContainerWindow::UpdateMenuContext)",
		"_ZN8BPrivate16BContainerWindow10UpdateMenuEP5BMenuNS0_17UpdateMenuContextE");
	TEST("icu_57::BreakIterator::registerInstance(icu_57::BreakIterator*, icu_57::Locale const&, UBreakIteratorType, UErrorCode&)",
		"_ZN6icu_5713BreakIterator16registerInstanceEPS0_RKNS_6LocaleE18UBreakIteratorTypeR10UErrorCode");
}
#undef TEST


/* static */ void
DemangleTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DemangleTest");

	suite.addTest(new CppUnit::TestCaller<DemangleTest>(
		"DemangleTest::RunGCC2Tests", &DemangleTest::RunGCC2Tests));
	suite.addTest(new CppUnit::TestCaller<DemangleTest>(
		"DemangleTest::RunGCC3+Tests", &DemangleTest::RunGCC3PTests));

	parent.addTest("DemangleTest", &suite);
}
