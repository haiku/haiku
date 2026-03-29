/*
 * Copyright 2022-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <TestSuiteAddon.h>

#include "Bitmap.h"


class BitmapTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(BitmapTest);
	CPPUNIT_TEST(Shift_CorrectBitSet);
	CPPUNIT_TEST(Multielement_Shift_CorrectBitSet);
	CPPUNIT_TEST(Resize_CorrectBitSet);
	CPPUNIT_TEST_SUITE_END();

public:
	void Shift_CorrectBitSet()
	{
		BKernel::Bitmap bitmap(20);
		bitmap.Set(6);

		CPPUNIT_ASSERT(bitmap.Get(6));
		CPPUNIT_ASSERT(!bitmap.Get(5));
		CPPUNIT_ASSERT(!bitmap.Get(7));

		bitmap.Shift(10);

		CPPUNIT_ASSERT(bitmap.Get(16));
		CPPUNIT_ASSERT(!bitmap.Get(15));
		CPPUNIT_ASSERT(!bitmap.Get(17));
		CPPUNIT_ASSERT(!bitmap.Get(6));

		bitmap.Shift(-9);

		CPPUNIT_ASSERT(bitmap.Get(7));
		CPPUNIT_ASSERT(!bitmap.Get(6));
		CPPUNIT_ASSERT(!bitmap.Get(8));
		CPPUNIT_ASSERT(!bitmap.Get(16));
	}

	void Multielement_Shift_CorrectBitSet() {
		BKernel::Bitmap bitmap(200);
		bitmap.Set(7);

		CPPUNIT_ASSERT(bitmap.Get(7));
		CPPUNIT_ASSERT(!bitmap.Get(6));

		bitmap.Shift(100);

		CPPUNIT_ASSERT(!bitmap.Get(7));
		CPPUNIT_ASSERT(bitmap.Get(107));
		CPPUNIT_ASSERT(!bitmap.Get(106));

		bitmap.Shift(-100);

		CPPUNIT_ASSERT(bitmap.Get(7));
		CPPUNIT_ASSERT(!bitmap.Get(107));
		CPPUNIT_ASSERT(!bitmap.Get(6));
	}

	void Resize_CorrectBitSet()
	{
		BKernel::Bitmap bitmap(10);
		bitmap.Set(6);

		CPPUNIT_ASSERT(bitmap.Get(6));
		CPPUNIT_ASSERT(!bitmap.Get(5));
		CPPUNIT_ASSERT(!bitmap.Get(7));

		bitmap.Resize(20);

		CPPUNIT_ASSERT(bitmap.Get(6));
		CPPUNIT_ASSERT(!bitmap.Get(7));
		CPPUNIT_ASSERT(!bitmap.Get(19));

		bitmap.Resize(200);
		bitmap.Set(199);

		CPPUNIT_ASSERT(bitmap.Get(6));
		CPPUNIT_ASSERT(!bitmap.Get(7));
		CPPUNIT_ASSERT(!bitmap.Get(19));
		CPPUNIT_ASSERT(bitmap.Get(199));
		CPPUNIT_ASSERT(!bitmap.Get(198));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(BitmapTest, getTestSuiteName());
