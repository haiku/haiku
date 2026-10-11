/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _STORAGE_BASIC_TEST_H_
#define _STORAGE_BASIC_TEST_H_


#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceTypes.h>
#include <fs_volume.h>
#include <Partition.h>
#include <SupportDefs.h>

#include <TestShell.h>
#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>

#include <set>
#include <string>


class BasicTest : public CppUnit::TestFixture {
public:
	BasicTest()
		:
		fAvailableFDs(0),
		fValidCWD(false)
	{
	}

	virtual ~BasicTest() {}

	void setUp() {

		fAvailableFDs = count_available_fds();
		SaveCWD();
	}

	void tearDown() {
		RestoreCWD();
		int32 availableFDs = count_available_fds();
		if (availableFDs != fAvailableFDs) {
			printf("WARNING: Number of available file descriptors has changed "
				   "during test: %d -> %d\n", (int)fAvailableFDs, (int)availableFDs);
			fAvailableFDs = availableFDs;
		}
	}

	static void execCommand(const std::string &cmdLine)
	{
		system(cmdLine.c_str());
	}

protected:
	int32 fAvailableFDs;
	bool fValidCWD;
	char fCurrentWorkingDir[B_PATH_NAME_LENGTH+1];

	void RestoreCWD(const char *alternate = NULL)
	{
		if (fValidCWD)
			chdir(fCurrentWorkingDir);
		else if (alternate != NULL)
			chdir(alternate);
	}

	void SaveCWD() {
		fValidCWD = getcwd(fCurrentWorkingDir, B_PATH_NAME_LENGTH);
	}

	BPartition* GetPartitionForPath(const char* imageFile, BDiskDevice &device)
	{
		BDiskDeviceRoster roster;
		BPartition* partition;
		partition_id id = roster.RegisterFileDevice(imageFile);
		if (id < 0) {
			return NULL;
		}
		if (roster.GetPartitionWithID(id, &device, &partition) != B_OK) {
			roster.UnregisterFileDevice(id);
			return NULL;
		}
		return partition;
	}

	std::string CreateVolume(const char* mountPoint, int32 megs, bool makeMountPoint = true)
	{
		std::string imageFile = CreateTemporaryFile(1024 * megs);

		if (makeMountPoint)
			mkdir(mountPoint, S_IRWXU | S_IRWXG | S_IRWXO);
#if 0
		std::stringstream str;
		str << "mkfs -q -t bfs " << imageFile << " StorageTest;";
		str << "sync;";
		str << "mount " << imageFile << " " << mountPoint;

		execCommand(str.str());
#else
		BDiskDevice device;
		BPartition* partition = GetPartitionForPath(imageFile.c_str(), device);

		device.PrepareModifications();
		partition->Initialize(kPartitionTypeBFS, "StorageTest", "");
		device.CommitModifications();

		partition = GetPartitionForPath(imageFile.c_str(), device);
		partition->Mount(mountPoint);
#endif
		return imageFile;
	}

	void DeleteVolume(std::string imageFile, const char* mountPoint, bool deleteMountPoint = true)
	{
#if 0
		std::stringstream str;
		str << "umount " << mountPoint;

		execCommand(str.str());
#else
		BDiskDeviceRoster roster;
		BDiskDevice device;
		BPartition* partition = GetPartitionForPath(imageFile.c_str(), device);
		partition->Unmount(B_FORCE_UNMOUNT);
		roster.UnregisterFileDevice(imageFile.c_str());
#endif
		if (deleteMountPoint)
			rmdir(mountPoint);
		unlink(imageFile.c_str());
	}

	std::string CreateTemporaryFile(ssize_t sizeInK = 0) {
		char tpl[] = "/tmp/UnitTester_storage_XXXXXX";
		int fd = mkstemp(tpl);
		if (fd < 0) {
			return std::string();
		}
		if (sizeInK > 0) {
			const int zero[256] = {};
			for (int i = 0; i < sizeInK; i++) {
				ssize_t written = write(fd, zero, sizeof(zero));
				if (written < static_cast<ssize_t>(sizeof(zero))) {
					close(fd);
					return std::string();
				}
			}
		}
		close(fd);
			// we don't need this file open
		return std::string(tpl);
	}

	int32
	count_available_fds()
	{
		std::set<int> fds;
		int fd;
		while ((fd = dup(1)) != -1)
			fds.insert(fd);
		for (std::set<int>::iterator it = fds.begin(); it != fds.end(); it++)
			close(*it);
		return fds.size();
	}
};



// Some other helpful stuff.

static
inline
bool
operator==(const struct stat &st1, const struct stat &st2)
{
	return (
		st1.st_dev == st2.st_dev
		&& st1.st_ino == st2.st_ino
		&& st1.st_mode == st2.st_mode
		&& st1.st_nlink == st2.st_nlink
		&& st1.st_uid == st2.st_uid
		&& st1.st_gid == st2.st_gid
		&& st1.st_size == st2.st_size
		&& st1.st_blksize == st2.st_blksize
		&& st1.st_atime == st2.st_atime
		&& st1.st_mtime == st2.st_mtime
		&& st1.st_ctime == st2.st_ctime
		&& st1.st_crtime == st2.st_crtime
	);
}

// first parameter is equal to the second or third
template<typename A, typename B, typename C>
static
inline
bool
equals(const A &a, const B &b, const C &c)
{
	return (a == b || a == c);
}

// A little helper class for tests. It works like a set of strings, that
// are marked tested or untested.
class TestSet {
public:
	typedef std::set<string> nameset;

public:
	TestSet()
	{
	}

	void add(std::string name)
	{
		if (fTestedNames.find(name) == fTestedNames.end())
			fUntestedNames.insert(name);
	}

	void remove(std::string name)
	{
		if (fUntestedNames.find(name) != fUntestedNames.end())
			fUntestedNames.erase(name);
		else if (fTestedNames.find(name) != fTestedNames.end())
			fTestedNames.erase(name);
	}

	void clear() {
		fUntestedNames.clear();
		fTestedNames.clear();
	}

	void rewind() {
		fUntestedNames.insert(fTestedNames.begin(), fTestedNames.end());
		fTestedNames.clear();
	}

	bool test(std::string name, bool dump = BTestShell::GlobalBeVerbose())
	{
		bool result = (fUntestedNames.find(name) != fUntestedNames.end());
		if (result) {
			fUntestedNames.erase(name);
			fTestedNames.insert(name);
		} else if (dump) {
			// dump untested
			printf("TestSet::test(`%s')\n", name.c_str());
			printf("untested:\n");
			for (nameset::iterator it = fUntestedNames.begin();
				 it != fUntestedNames.end();
				 ++it) {
				printf("  `%s'\n", it->c_str());
			}
		}
		return result;
	}

	bool testDone()
	{
		return (fUntestedNames.empty());
	}

private:
	nameset	fUntestedNames;
	nameset	fTestedNames;
};

#endif // _STORAGE_BASIC_TEST_H_
