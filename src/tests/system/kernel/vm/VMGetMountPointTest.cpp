/*
 * Copyright 2018 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "VMGetMountPointTest.h"

#include <string.h>

#include <fs/KPath.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestUtils.h>


// Kernel stubs


extern "C" team_id
team_get_kernel_team_id(void)
{
	return 0;
}


extern "C" team_id
team_get_current_team_id(void)
{
	return 0;
}


extern "C" status_t
vfs_normalize_path(const char* path, char* buffer, size_t bufferSize,
	bool traverseLink, bool kernel)
{
	return B_NOT_SUPPORTED;
}

struct stat;

extern "C" int
stat(const char* path, struct stat* s)
{
	if(strcmp(path, "/testduplicate") == 0)
		return 0;
	else
		return -1;
}

namespace BPrivate {
namespace DiskDevice {

class KPartition {
public:
	KPartition(std::string name, std::string contentName, bool containsFilesystem)
		: fName(name), fContentName(contentName), fContainsFilesystem(containsFilesystem)
		{}

	const char *Name() const;
	const char *ContentName() const;
	bool ContainsFileSystem() const;

private:
	std::string fName;
	std::string fContentName;
	bool fContainsFilesystem;
};


const char *
KPartition::Name() const
{
	return fName.c_str();
}


const char *
KPartition::ContentName() const
{
	return fContentName.c_str();
}


bool
KPartition::ContainsFileSystem() const
{
	return fContainsFilesystem;
}

}
}


using BPrivate::DiskDevice::KPartition;


status_t
get_mount_point(KPartition* partition, KPath* mountPoint);


//	#pragma mark -


VMGetMountPointTest::VMGetMountPointTest(std::string name)
	: BTestCase(name)
{
}

#define ADD_TEST(s, cls, m) \
	s->addTest(new CppUnit::TestCaller<cls>(#cls "::" #m, &cls::m));


CppUnit::Test*
VMGetMountPointTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("VMGetMountPointTest");

	ADD_TEST(suite, VMGetMountPointTest, TestNullMountPointReturnsBadValue);
	ADD_TEST(suite, VMGetMountPointTest, TestPartitionWithoutFilesystemReturnsBadValue);
	ADD_TEST(suite, VMGetMountPointTest, TestPartitionContentNameUsedFirst);
	ADD_TEST(suite, VMGetMountPointTest, TestPartitionNameUsedSecond);
	ADD_TEST(suite, VMGetMountPointTest, TestPartitionWithoutAnyNameIsNotRoot);
	ADD_TEST(suite, VMGetMountPointTest, TestPartitionNameWithSlashesRemoved);
	ADD_TEST(suite, VMGetMountPointTest, TestPartitionMountPointExists);

	return suite;
}


void
VMGetMountPointTest::TestNullMountPointReturnsBadValue()
{
	status_t status = get_mount_point(NULL, NULL);

	CPPUNIT_ASSERT_EQUAL(status, B_BAD_VALUE);
}


void
VMGetMountPointTest::TestPartitionWithoutFilesystemReturnsBadValue()
{
	KPartition partition("", "", false);
	KPath path;

	status_t status = get_mount_point(&partition, &path);

	CPPUNIT_ASSERT_EQUAL(status, B_BAD_VALUE);
}


void
VMGetMountPointTest::TestPartitionContentNameUsedFirst()
{
	KPartition partition("test1", "test2", true);
	KPath path;

	status_t status = get_mount_point(&partition, &path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/test2") == 0);
}


void
VMGetMountPointTest::TestPartitionNameUsedSecond()
{
	KPartition partition("test1", "", true);
	KPath path;

	status_t status = get_mount_point(&partition, &path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/test1") == 0);
}


void
VMGetMountPointTest::TestPartitionWithoutAnyNameIsNotRoot()
{
	KPartition partition("", "", true);
	KPath path;

	status_t status = get_mount_point(&partition, &path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/") != 0);
}


void
VMGetMountPointTest::TestPartitionNameWithSlashesRemoved()
{
	KPartition partition("", "testing/slashes", true);
	KPath path;

	status_t status = get_mount_point(&partition, &path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/testing/slashes") != 0);
}


void
VMGetMountPointTest::TestPartitionMountPointExists()
{
	KPartition partition("", "testduplicate", true);
	KPath path;

	status_t status = get_mount_point(&partition, &path);

	CPPUNIT_ASSERT_EQUAL(status, B_OK);
	CPPUNIT_ASSERT(strcmp(path.Path(), "/testduplicate") != 0);
}


