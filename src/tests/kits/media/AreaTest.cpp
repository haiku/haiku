/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cstddef>


class AreaTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(AreaTest);
	CPPUNIT_TEST(IntPtr_AreaFor_PtrOffsetInsideArea);
	CPPUNIT_TEST(IntPtr_Clone_ReturnedAreasHaveDifferentIDs);
	CPPUNIT_TEST(IntPtr_Clone_ReturnedAreasHaveDifferentAddresses);
	CPPUNIT_TEST(IntPtr_Clone_ReturnedMemoryIsSynchronized);
	CPPUNIT_TEST_SUITE_END();

	struct AreaData {
		int* ptr;
		char* addr;
		area_id id;
	} fArea, fClone1, fClone2;
	ptrdiff_t fOffset;

public:
	void setUp()
	{
		fArea.ptr = new int[1];
		fArea.id = area_for(fArea.ptr);
		area_info info;
		get_area_info(fArea.id, &info);
		fArea.addr = reinterpret_cast<char*>(info.address);

		fClone1.id = clone_area("clone 1", reinterpret_cast<void**>(&fClone1.addr), B_ANY_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, fArea.id);
		fClone2.id = clone_area("clone 2", reinterpret_cast<void**>(&fClone2.addr), B_ANY_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, fArea.id);

		fOffset = reinterpret_cast<ptrdiff_t>(fArea.ptr) - reinterpret_cast<ptrdiff_t>(fArea.addr);

		fClone1.ptr = reinterpret_cast<int*>(fClone1.addr + fOffset);
		fClone2.ptr = reinterpret_cast<int*>(fClone2.addr + fOffset);
	}

	void tearDown()
	{
		delete[] fArea.ptr;
		delete_area(fClone1.id);
		delete_area(fClone2.id);
	}

	void IntPtr_AreaFor_PtrOffsetInsideArea()
	{
		CPPUNIT_ASSERT(fOffset >= 0);
	}

	void IntPtr_Clone_ReturnedAreasHaveDifferentIDs()
	{
		CPPUNIT_ASSERT(fArea.id != fClone1.id);
		CPPUNIT_ASSERT(fArea.id != fClone2.id);
		CPPUNIT_ASSERT(fClone1.id != fClone2.id);
	}

	void IntPtr_Clone_ReturnedAreasHaveDifferentAddresses()
	{
		CPPUNIT_ASSERT(fArea.addr != fClone1.addr);
		CPPUNIT_ASSERT(fArea.addr != fClone2.addr);
		CPPUNIT_ASSERT(fClone1.addr != fClone2.addr);
	}

	void IntPtr_Clone_ReturnedMemoryIsSynchronized()
	{
		fArea.ptr[0] = 0x12345678;
		CPPUNIT_ASSERT(fArea.ptr[0] == fClone1.ptr[0]);
		CPPUNIT_ASSERT(fClone2.ptr[0] == fClone1.ptr[0]);
		CPPUNIT_ASSERT_EQUAL(0x12345678, fClone2.ptr[0]);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AreaTest, getTestSuiteName());
