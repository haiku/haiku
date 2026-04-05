/*
 * Copyright 2018-2026, Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <fs/KPath.h>
#include <slab/Slab.h>
#include <disk_device_manager/KPartition.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <TestSuiteAddon.h>


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


object_cache* sPathNameCache;


class KPartitionGetMountPointTest : public CppUnit::TestFixture {
public:
	CPPUNIT_TEST_SUITE(KPartitionGetMountPointTest);
	CPPUNIT_TEST(WithoutFilesystem_ReturnsBadValue);
	CPPUNIT_TEST(DifferentNameAndContentName_ContentNameTakesPrecedence);
	CPPUNIT_TEST(ContentNameEmpty_NameUsed);
	CPPUNIT_TEST(NameAndContentNameEmpty_NotRoot);
	CPPUNIT_TEST(NameWithSlashes_SlashesAreRemoved);
	CPPUNIT_TEST(MountPointExists_UsesDifferentPath);
	CPPUNIT_TEST_SUITE_END();

public:
	void WithoutFilesystem_ReturnsBadValue()
	{
		KPartition partition;
		partition.SetName("");
		partition.SetContentName("");
		partition.SetFlags(0);

		KPath path;
		status_t status = partition.GetMountPoint(&path);

		CPPUNIT_ASSERT_EQUAL(status, B_BAD_VALUE);
	}

	void DifferentNameAndContentName_ContentNameTakesPrecedence()
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

	void ContentNameEmpty_NameUsed()
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

	void NameAndContentNameEmpty_NotRoot()
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

	void NameWithSlashes_SlashesAreRemoved()
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

	void MountPointExists_UsesDifferentPath()
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
};

// FIXME: hangs
//CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(KPartitionGetMountPointTest, getTestSuiteName());
