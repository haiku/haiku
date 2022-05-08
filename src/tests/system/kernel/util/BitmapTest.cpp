#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <TestUtils.h>

#include "BitmapTest.h"
#include "Bitmap.h"

BitmapTest::BitmapTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
BitmapTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("Bitmap");

	suite->addTest(new CppUnit::TestCaller<BitmapTest>("Bitmap::Resize test",
		&BitmapTest::ResizeTest));
	suite->addTest(new CppUnit::TestCaller<BitmapTest>("Bitmap::Shift test",
		&BitmapTest::ShiftTest));

	return suite;
}

void
BitmapTest::ResizeTest()
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

void
BitmapTest::ShiftTest()
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

	// Now test cross-element shifting.
	bitmap.Resize(200);

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
