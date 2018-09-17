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
	TEST("", "()");
	TEST("BPrivate::IconCache::SyncDraw(BPrivate::Model*, BView*, BPoint, BPrivate::IconDrawMode, icon_size, void*, void*)",
		"SyncDraw__Q28BPrivate9IconCachePQ28BPrivate5ModelP5BViewG6BPointQ28BPrivate12IconDrawMode9icon_sizePFP5BViewG6BPointP7BBitmapPv_vPv");
}


void
DemangleTest::RunGCC3PTests()
{
	TEST("BPrivate::IconCache::SyncDraw(BPrivate::Model*, BView*, BPoint, BPrivate::IconDrawMode, icon_size, void (*)(BView*, BPoint, BBitmap*, void*), void*)",
		"_ZN8BPrivate9IconCache8SyncDrawEPNS_5ModelEP5BView6BPointNS_12IconDrawModeE9icon_sizePFvS4_S5_P7BBitmapPvESA_");
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
