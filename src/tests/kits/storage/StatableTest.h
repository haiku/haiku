/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _STORAGE_STATABLE_TEST_H_
#define _STORAGE_STATABLE_TEST_H_


#include <sys/stat.h>

#include <vector>

#include <Statable.h>
#include <Entry.h>
#include <Node.h>
#include <Volume.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"


template <typename C>
struct TestEntries
{
	~TestEntries()
	{
		delete_all();
	}

	void delete_all() {
		for (typename std::vector<C*>::iterator it = entries.begin();
			 it != entries.end();
			 it++) {
			// Arghh, BStatable has no virtual destructor!
			// Workaround: try to cast to one of the subclasses
			if (BNode *node = dynamic_cast<BNode*>(*it))
				delete node;
			else if (BEntry *entry = dynamic_cast<BEntry*>(*it))
				delete entry;
			else
				delete *it;
		}
		clear();
	}

	void clear() {
		entries.clear();
		entryNames.clear();
		rewind();
	}

	void add(C* entry, std::string entryName)
	{
		entries.push_back(entry);
		entryNames.push_back(entryName);
	}

	bool getNext(C *&entry, std::string &entryName)
	{
		bool result = (entryIt != entries.end()
					   && entryNameIt != entryNames.end());
		if (result) {
			entry = *entryIt;
			entryName = *entryNameIt;
			entryIt++;
			entryNameIt++;
		}
		return result;
	}

	void rewind() {
		entryIt = entries.begin();
		entryNameIt = entryNames.begin();
	}

	std::vector<C*>						entries;
	std::vector<std::string>			entryNames;
	typename std::vector<C*>::iterator			entryIt;
	typename std::vector<std::string>::iterator	entryNameIt;
};

typedef TestEntries<BStatable> TestStatables;


class StatableTest : public BasicTest {
	// This class is not registered because it is abstract.
	// Derived classes should use CPPUNIT_TEST_SUB_SUITE(Derived, StatableTest).
	CPPUNIT_TEST_SUITE(StatableTest);
	CPPUNIT_TEST(ExistingEntries_SetXYZ_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetXYZ_ReturnsError);
	CPPUNIT_TEST(ExistingEntries_IsXYZ_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_IsXYZ_ReturnsFalse);
	CPPUNIT_TEST(ExistingEntries_GetXYZ_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_GetXYZ_ReturnsNoInit);
	CPPUNIT_TEST(BadArgs_GetXYZ_ReturnsError);
	CPPUNIT_TEST(ExistingEntries_GetStat_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_GetStat_ReturnsNoInit);
	CPPUNIT_TEST(BadArgs_GetStat_ReturnsError);
	CPPUNIT_TEST_SUITE_END();

protected:
	virtual void CreateROStatables(TestStatables& testEntries) {}
	virtual void CreateRWStatables(TestStatables& testEntries) {}
	virtual void CreateUninitializedStatables(TestStatables& testEntries) {}

public:
	void setUp() {
		BasicTest::setUp();
	}

	void tearDown() {
		BasicTest::tearDown();
	}

	void ExistingEntries_SetXYZ_ReturnsOk() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateRWStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			struct stat st;
			uid_t owner = 0xdad;
			gid_t group = 0xdee;
			mode_t perms = 0x0ab;	// -w- r-x -wx	-- unusual enough? ;-)
			time_t mtime = 1234567;
			time_t ctime = 654321;
	// R5: access time unused
	#if !TEST_R5 && !TEST_OBOS /* !!!POSIX ONLY!!! */
			time_t atime = 2345678;
	#endif
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->SetOwner(owner));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->SetGroup(group));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->SetPermissions(perms));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->SetModificationTime(mtime));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->SetCreationTime(ctime));
	#if !TEST_R5 && !TEST_OBOS /* !!!POSIX ONLY!!! */
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->SetAccessTime(atime));
	#endif
			CPPUNIT_ASSERT(lstat(entryName.c_str(), &st) == 0);
			CPPUNIT_ASSERT(owner == st.st_uid);
			CPPUNIT_ASSERT(group == st.st_gid);
			CPPUNIT_ASSERT(perms == (st.st_mode & S_IUMSK));
			CPPUNIT_ASSERT(mtime == st.st_mtime);
			CPPUNIT_ASSERT(ctime == st.st_crtime);
	#if !TEST_R5 && !TEST_OBOS /* !!!POSIX ONLY!!! */
			CPPUNIT_ASSERT(atime == st.st_atime);
	#endif
		}
		testEntries.delete_all();
	}

	void Uninitialized_SetXYZ_ReturnsError() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateUninitializedStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			uid_t owner = 0xdad;
			gid_t group = 0xdee;
			mode_t perms = 0x0ab;	// -w- r-x -wx	-- unusual enough? ;-)
			time_t mtime = 1234567;
			time_t ctime = 654321;
			time_t atime = 2345678;
			CPPUNIT_ASSERT(statable->SetOwner(owner) != B_OK);
			CPPUNIT_ASSERT(statable->SetGroup(group) != B_OK);
			CPPUNIT_ASSERT(statable->SetPermissions(perms) != B_OK);
			CPPUNIT_ASSERT(statable->SetModificationTime(mtime) != B_OK);
			CPPUNIT_ASSERT(statable->SetCreationTime(ctime) != B_OK);
			CPPUNIT_ASSERT(statable->SetAccessTime(atime) != B_OK);
		}
		testEntries.delete_all();
	}

	void ExistingEntries_IsXYZ_ReturnsOk() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateROStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			struct stat st;
			CPPUNIT_ASSERT(lstat(entryName.c_str(), &st) == 0);
			CPPUNIT_ASSERT(statable->IsDirectory() == S_ISDIR(st.st_mode));
			CPPUNIT_ASSERT(statable->IsFile() == S_ISREG(st.st_mode));
			CPPUNIT_ASSERT(statable->IsSymLink() == S_ISLNK(st.st_mode));
		}
		testEntries.delete_all();
	}

	void Uninitialized_IsXYZ_ReturnsFalse() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateUninitializedStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			CPPUNIT_ASSERT(statable->IsDirectory() == false);
			CPPUNIT_ASSERT(statable->IsFile() == false);
			CPPUNIT_ASSERT(statable->IsSymLink() == false);
		}
		testEntries.delete_all();
	}

	void ExistingEntries_GetXYZ_ReturnsOk() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateROStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			struct stat st;
			node_ref ref;
			uid_t owner;
			gid_t group;
			mode_t perms;
			off_t size;
			time_t mtime;
			time_t ctime;
	// R5: access time unused
	#if !TEST_R5 && !TEST_OBOS /* !!!POSIX ONLY!!! */
			time_t atime;
	#endif
			BVolume volume;
			CPPUNIT_ASSERT(lstat(entryName.c_str(), &st) == 0);
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetNodeRef(&ref));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetOwner(&owner));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetGroup(&group));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetPermissions(&perms));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetSize(&size));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetModificationTime(&mtime));
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetCreationTime(&ctime));
	#if !TEST_R5 && !TEST_OBOS /* !!!POSIX ONLY!!! */
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetAccessTime(&atime));
	#endif
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetVolume(&volume));
			CPPUNIT_ASSERT(ref.device == st.st_dev && ref.node == st.st_ino);
			CPPUNIT_ASSERT(owner == st.st_uid);
			CPPUNIT_ASSERT(group == st.st_gid);
	// R5: returns not only the permission bits, so we need to filter for the test
	//		CPPUNIT_ASSERT(perms == (st.st_mode & S_IUMSK));
			CPPUNIT_ASSERT((perms & S_IUMSK) == (st.st_mode & S_IUMSK));
			CPPUNIT_ASSERT(size == st.st_size);
			CPPUNIT_ASSERT(mtime == st.st_mtime);
			CPPUNIT_ASSERT(ctime == st.st_crtime);
	#if !TEST_R5 && !TEST_OBOS /* !!!POSIX ONLY!!! */
			CPPUNIT_ASSERT(atime == st.st_atime);
	#endif
			CPPUNIT_ASSERT(volume == BVolume(st.st_dev));
		}
		testEntries.delete_all();
	}

	void Uninitialized_GetXYZ_ReturnsNoInit() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateUninitializedStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			node_ref ref;
			uid_t owner;
			gid_t group;
			mode_t perms;
			off_t size;
			time_t mtime;
			time_t ctime;
			time_t atime;
			BVolume volume;
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetNodeRef(&ref));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetOwner(&owner));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetGroup(&group));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetPermissions(&perms));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetSize(&size));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetModificationTime(&mtime));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetCreationTime(&ctime));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetAccessTime(&atime));
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetVolume(&volume));
		}
		testEntries.delete_all();
	}

	void BadArgs_GetXYZ_ReturnsError() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateROStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
	// R5: crashs, if passing NULL to any of these methods
	#if !TEST_R5
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetNodeRef(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetOwner(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetGroup(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetPermissions(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetSize(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetModificationTime(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetCreationTime(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetAccessTime(NULL));
			CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, statable->GetVolume(NULL));
	#endif
		}
		testEntries.delete_all();
	}


	void ExistingEntries_GetStat_ReturnsOk() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateROStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			struct stat st1, st2;
			CPPUNIT_ASSERT_EQUAL(B_OK, statable->GetStat(&st1));
			CPPUNIT_ASSERT(lstat(entryName.c_str(), &st2) == 0);
			CPPUNIT_ASSERT(st1 == st2);
		}
		testEntries.delete_all();
	}

	void Uninitialized_GetStat_ReturnsNoInit() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateUninitializedStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); ) {
			struct stat st1;
			CPPUNIT_ASSERT_EQUAL(B_NO_INIT, statable->GetStat(&st1));
		}
		testEntries.delete_all();
	}


	void BadArgs_GetStat_ReturnsError() {
		TestStatables testEntries;
		BStatable *statable;
		std::string entryName;
		CreateROStatables(testEntries);
		for (testEntries.rewind(); testEntries.getNext(statable, entryName); )
			CPPUNIT_ASSERT(statable->GetStat(NULL) != B_OK);
		testEntries.delete_all();
	}
};


#endif // _STORAGE_STATABLE_TEST_H_
