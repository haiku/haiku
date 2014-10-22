/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "AreaTest.h"

#include <OS.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


AreaTest::AreaTest()
{
}


AreaTest::~AreaTest()
{
}


void
AreaTest::TestAreas()
{
	int * ptr = new int[1];
	char *adr;
	area_id id;
	ptrdiff_t offset;

	area_info info;
	id = area_for(ptr);
	get_area_info(id, &info);
	adr = (char *)info.address;
	offset = (ptrdiff_t)ptr - (ptrdiff_t)adr;


	char * adrclone1;
	char * adrclone2;
	int * ptrclone1;
	int * ptrclone2;
	area_id idclone1;
	area_id idclone2;

	idclone1 = clone_area("clone 1", (void **)&adrclone1, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, id);
	idclone2 = clone_area("clone 2", (void **)&adrclone2, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, id);

	ptrclone1 = (int *)(adrclone1 + offset);
	ptrclone2 = (int *)(adrclone2 + offset);

	// Check that he pointer is inside the area returned by area_for...
	CPPUNIT_ASSERT(offset >= 0);

	// Chech that the clones have different IDs
	CPPUNIT_ASSERT(id != idclone1);
	CPPUNIT_ASSERT(id != idclone2);
	CPPUNIT_ASSERT(idclone1 != idclone2);

	// Check that the clones have different addresses
	CPPUNIT_ASSERT(adr != adrclone1);
	CPPUNIT_ASSERT(adr != adrclone2);
	CPPUNIT_ASSERT(adrclone1 != adrclone2);

	// Check that changes on one view of the area are visible in others.
	ptr[0] = 0x12345678;
	CPPUNIT_ASSERT(ptr[0] == ptrclone1[0]);
	CPPUNIT_ASSERT(ptrclone2[0] == ptrclone1[0]);
	CPPUNIT_ASSERT_EQUAL(0x12345678, ptrclone2[0]);
}


/*static*/ void
AreaTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("AreaTest");

	suite.addTest(new CppUnit::TestCaller<AreaTest>(
		"AreaTest::TestAreas", &AreaTest::TestAreas));

	parent.addTest("AreaTest", &suite);
}
