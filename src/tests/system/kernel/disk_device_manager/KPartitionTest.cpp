/*
 * Copyright 2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "KPartitionTest.h"

#include <string.h>

#include <fs/KPath.h>
#include <disk_device_manager/KPartition.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestUtils.h>


struct stat;

extern "C" int
stat(const char* path, struct stat* s)
{
	if(strcmp(path, "/testduplicate") == 0)
		return 0;
	else
		return -1;
}

using BPrivate::DiskDevice::KPartition;


//	#pragma mark -


KPartitionGetMountPointTest::KPartitionGetMountPointTest(std::string name)
	: BTestCase(name)
{
}

#define ADD_TEST(s, cls, m) \
	s->addTest(new CppUnit::TestCaller<cls>(#cls "::" #m, &cls::m));


CppUnit::Test*
KPartitionGetMountPointTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("KPartitionGetMountPointTest");

	ADD_TEST(suite, KPartitionGetMountPointTest, TestPartitionWithoutFilesystemReturnsBadValue);
	ADD_TEST(suite, KPartitionGetMountPointTest, TestPartitionContentNameUsedFirst);
	ADD_TEST(suite, KPartitionGetMountPointTest, TestPartitionNameUsedSecond);
	ADD_TEST(suite, KPartitionGetMountPointTest, TestPartitionWithoutAnyNameIsNotRoot);
	ADD_TEST(suite, KPartitionGetMountPointTest, TestPartitionNameWithSlashesRemoved);
	ADD_TEST(suite, KPartitionGetMountPointTest, TestPartitionMountPointExists);

	return suite;
}


void
KPartitionGetMountPointTest::TestPartitionWithoutFilesystemReturnsBadValue()
{
	KPartition partition;
	partition.SetName("");
	partition.SetContentName("");
	partition.SetFlags(0);

	KPath path;
	status_t status = partition.GetMountPoint(&path);

	CPPUNIT_ASSERT_EQUAL(status, B_BAD_VALUE);
}


void
KPartitionGetMountPointTest::TestPartitionContentNameUsedFirst()
{
	KPartition partition;
	partition.SetName("test1");
	partition.SetContentName("test2");
	partition.SetFlags(B_PARTITION_FILE_SYSTEM);

	KPath path;
	status_t status = partition.GetMountPoint(&path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/test2") == 0);
}


void
KPartitionGetMountPointTest::TestPartitionNameUsedSecond()
{
	KPartition partition;
	partition.SetName("test1");
	partition.SetContentName("");
	partition.SetFlags(B_PARTITION_FILE_SYSTEM);

	KPath path;
	status_t status = partition.GetMountPoint(&path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/test1") == 0);
}


void
KPartitionGetMountPointTest::TestPartitionWithoutAnyNameIsNotRoot()
{
	KPartition partition;
	partition.SetName("");
	partition.SetContentName("");
	partition.SetFlags(B_PARTITION_FILE_SYSTEM);

	KPath path;
	status_t status = partition.GetMountPoint(&path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/") != 0);
}


void
KPartitionGetMountPointTest::TestPartitionNameWithSlashesRemoved()
{
	KPartition partition;
	partition.SetName("");
	partition.SetContentName("testing/slashes");
	partition.SetFlags(B_PARTITION_FILE_SYSTEM);

	KPath path;
	status_t status = partition.GetMountPoint(&path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/testing/slashes") != 0);
}


void
KPartitionGetMountPointTest::TestPartitionMountPointExists()
{
	KPartition partition;
	partition.SetName("");
	partition.SetContentName("testduplicate");
	partition.SetFlags(B_PARTITION_FILE_SYSTEM);

	KPath path;
	status_t status = partition.GetMountPoint(&path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/testduplicate") != 0);
}
