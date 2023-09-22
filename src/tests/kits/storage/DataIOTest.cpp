// DataIOTest.cpp

#include <string.h>
#include <BufferedDataIO.h>

#include <TestShell.h>

#include "DataIOTest.h"


CppUnit::Test*
DataIOTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<DataIOTest> TC;

	suite->addTest(new TC("BResourceString::BufferedDataIO Test",
		&DataIOTest::BufferedDataIOTest));

	return suite;
}


void
DataIOTest::BufferedDataIOTest()
{
	// very basic test
	NextSubTest();
	{
		BMallocIO mallocIO;
		CPPUNIT_ASSERT(mallocIO.SetSize(1024) == B_OK);

		BBufferedDataIO bufferedDataIO(mallocIO, 8, false);
		CPPUNIT_ASSERT(bufferedDataIO.InitCheck() == B_OK);

		CPPUNIT_ASSERT(bufferedDataIO.Write("test ", 5) == 5);
		CPPUNIT_ASSERT(bufferedDataIO.Write("test ", 5) == 5);
		CPPUNIT_ASSERT(bufferedDataIO.Write("test ", 5) == 5);

		CPPUNIT_ASSERT(bufferedDataIO.Flush() == B_OK);

		CPPUNIT_ASSERT(bufferedDataIO.Write("longer-test", 12) == 12);

		CPPUNIT_ASSERT(mallocIO.Position() == 27);
		CPPUNIT_ASSERT(memcmp(mallocIO.Buffer(), "test test test longer-test", 27) == 0);
	}
}
