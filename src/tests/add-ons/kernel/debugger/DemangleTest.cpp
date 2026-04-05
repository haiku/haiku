/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <String.h>
#include "Demangler.h"
#include "demangle.h"


class DemangleTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DemangleTest);
	CPPUNIT_TEST(GCC2);
	CPPUNIT_TEST(GCC3P);
	CPPUNIT_TEST_SUITE_END();

public:
	void GCC2()
	{
		// Long and complex things
		CPPUNIT_ASSERT_EQUAL(
			BString("BPrivate::IconCache::SyncDraw(BPrivate::Model*, BView*, BPoint, BPrivate::IconDrawMode, icon_size, void*, void*)"),
			Demangler::Demangle("SyncDraw__Q28BPrivate9IconCachePQ28BPrivate5ModelP5BViewG6BPointQ28BPrivate12IconDrawMode9icon_sizePFP5BViewG6BPointP7BBitmapPv_vPv"));
		CPPUNIT_ASSERT_EQUAL(
			BString("BPrivate::BContainerWindow::UpdateMenu(BMenu*, BPrivate::BContainerWindow::UpdateMenuContext)"),
			Demangler::Demangle("UpdateMenu__Q28BPrivate16BContainerWindowP5BMenuQ38BPrivate16BContainerWindow17UpdateMenuContext"));
		CPPUNIT_ASSERT_EQUAL(
			BString("icu_57::BreakIterator::registerInstance(icu_57::BreakIterator*, icu_57::Locale&, UBreakIteratorType, UErrorCode&)"),
			Demangler::Demangle("registerInstance__Q26icu_5713BreakIteratorPQ26icu_5713BreakIteratorRCQ26icu_576Locale18UBreakIteratorTypeR10UErrorCode"));

		// Previously caused crashes
		CPPUNIT_ASSERT_EQUAL(
			BString("_GLOBAL_::SetTo()"),
			Demangler::Demangle("SetTo__Q282_GLOBAL_"));
	}

	void GCC3P()
	{
		// Long and complex things
		CPPUNIT_ASSERT_EQUAL(
			BString("BPrivate::IconCache::SyncDraw(BPrivate::Model*, BView*, BPoint, BPrivate::IconDrawMode, icon_size, void (*)(BView*, BPoint, BBitmap*, void*), void*)"),
			Demangler::Demangle("_ZN8BPrivate9IconCache8SyncDrawEPNS_5ModelEP5BView6BPointNS_12IconDrawModeE9icon_sizePFvS4_S5_P7BBitmapPvESA_"));
		CPPUNIT_ASSERT_EQUAL(
			BString("BPrivate::BContainerWindow::UpdateMenu(BMenu*, BPrivate::BContainerWindow::UpdateMenuContext)"),
			Demangler::Demangle("_ZN8BPrivate16BContainerWindow10UpdateMenuEP5BMenuNS0_17UpdateMenuContextE"));
		CPPUNIT_ASSERT_EQUAL(
			BString("icu_57::BreakIterator::registerInstance(icu_57::BreakIterator*, icu_57::Locale const&, UBreakIteratorType, UErrorCode&)"),
			Demangler::Demangle("_ZN6icu_5713BreakIterator16registerInstanceEPS0_RKNS_6LocaleE18UBreakIteratorTypeR10UErrorCode"));
		CPPUNIT_ASSERT_EQUAL(
			BString("void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [clone .isra.25]"),
			Demangler::Demangle("_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE12_M_constructIPcEEvT_S7_St20forward_iterator_tag.isra.25"));
		CPPUNIT_ASSERT_EQUAL(
			BString("foo(int) [clone .part.1.123456] [clone .constprop.777.54321]"),
			Demangler::Demangle("_Z3fooi.part.1.123456.constprop.777.54321"));

		// Names independent of the full symbol
		char buffer[1024];
		demangle_symbol_gcc3("_Z3fooi.part.1.123456.constprop.777.1", buffer, sizeof(buffer), NULL);
		CPPUNIT_ASSERT_EQUAL(BString("foo[clone .part.1.123456] [clone .constprop.777.1] "), BString(buffer));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DemangleTest, getTestSuiteName());
