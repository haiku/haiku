/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <string>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <SymLink.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "NodeTest.h"


class SymLinkTest : public NodeTest {
	CPPUNIT_TEST_SUB_SUITE(SymLinkTest, NodeTest);
	CPPUNIT_TEST(Uninitialized_Constructor_ReturnsNoInit);
	CPPUNIT_TEST(FileLink_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExisting_PathConstructor_ReturnsEntryNotFound);
	CPPUNIT_TEST(Null_PathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(Empty_PathConstructor_ReturnsEntryNotFound);
	CPPUNIT_TEST(ExistingFile_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(TooLong_PathConstructor_ReturnsNameTooLong);

	CPPUNIT_TEST(DirectoryLink_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExisting_EntryConstructor_ReturnsEntryNotFound);
	CPPUNIT_TEST(Null_EntryConstructor_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_EntryConstructor_ReturnsError);
	CPPUNIT_TEST(ExistingFile_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(TooLong_EntryConstructor_ReturnsError);

	CPPUNIT_TEST(DirectoryLink_EntryRefConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExisting_EntryRefConstructor_ReturnsEntryNotFound);
	CPPUNIT_TEST(Null_EntryRefConstructor_ReturnsBadValue);
	CPPUNIT_TEST(ExistingFile_EntryRefConstructor_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_EntryRefConstructor_ReturnsOk);

	CPPUNIT_TEST(RelativePath_DirAndPathConstructor_ReturnsOk);
	CPPUNIT_TEST(InvalidPath_DirAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NonExisting_DirAndPathConstructor_ReturnsEntryNotFound);
	CPPUNIT_TEST(NullArgs_DirAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullDir_DirAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullPath_DirAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(EmptyPath_DirAndPathConstructor_ReturnsEntryNotFound);
	CPPUNIT_TEST(ExistingFile_DirAndPathConstructor_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_DirAndPathConstructor_ReturnsOk);
	CPPUNIT_TEST(TooLong_DirAndPathConstructor_ReturnsNameTooLong);
	CPPUNIT_TEST(FileSuperDir_DirAndPathConstructor_ReturnsEntryNotFound);

	CPPUNIT_TEST(FileLink_SetToPath_ReturnsOk);
	CPPUNIT_TEST(NonExisting_SetToPath_ReturnsEntryNotFound);
	CPPUNIT_TEST(Null_SetToPath_ReturnsBadValue);
	CPPUNIT_TEST(Empty_SetToPath_ReturnsEntryNotFound);
	CPPUNIT_TEST(ExistingFile_SetToPath_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_SetToPath_ReturnsOk);
	CPPUNIT_TEST(TooLong_SetToPath_ReturnsNameTooLong);

	CPPUNIT_TEST(DirectoryLink_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(NonExisting_SetToEntry_ReturnsEntryNotFound);
	CPPUNIT_TEST(Null_SetToEntry_ReturnsBadValue);
	CPPUNIT_TEST(Uninitialized_SetToEntry_ReturnsError);
	CPPUNIT_TEST(ExistingFile_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(TooLong_SetToEntry_ReturnsError);

	CPPUNIT_TEST(DirectoryLink_SetToEntryRef_ReturnsOk);
	CPPUNIT_TEST(NonExisting_SetToEntryRef_ReturnsEntryNotFound);
	CPPUNIT_TEST(Null_SetToEntryRef_ReturnsBadValue);
	CPPUNIT_TEST(ExistingFile_SetToEntryRef_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_SetToEntryRef_ReturnsOk);

	CPPUNIT_TEST(RelativePath_SetToDirAndPath_ReturnsOk);
	CPPUNIT_TEST(InvalidPath_SetToDirAndPath_ReturnsBadValue);
	CPPUNIT_TEST(NonExisting_SetToDirAndPath_ReturnsEntryNotFound);
	CPPUNIT_TEST(NullArgs_SetToDirAndPath_ReturnsBadValue);
	CPPUNIT_TEST(NullDir_SetToDirAndPath_ReturnsBadValue);
	CPPUNIT_TEST(NullPath_SetToDirAndPath_ReturnsBadValue);
	CPPUNIT_TEST(EmptyPath_SetToDirAndPath_ReturnsEntryNotFound);
	CPPUNIT_TEST(ExistingFile_SetToDirAndPath_ReturnsOk);
	CPPUNIT_TEST(ExistingDir_SetToDirAndPath_ReturnsOk);
	CPPUNIT_TEST(TooLong_SetToDirAndPath_ReturnsNameTooLong);
	CPPUNIT_TEST(FileSuperDir_SetToDirAndPath_ReturnsEntryNotFound);

	CPPUNIT_TEST(Uninitialized_ReadLink_ReturnsError);
	CPPUNIT_TEST(ExistingDirLink_ReadLink_ReturnsOk);
	CPPUNIT_TEST(ExistingFileLink_ReadLink_ReturnsOk);
	CPPUNIT_TEST(ExistingCyclicLink_ReadLink_ReturnsOk);
	CPPUNIT_TEST(ExistingLinkToNonExisting_ReadLink_ReturnsOk);
	CPPUNIT_TEST(NonExistingLink_ReadLink_ReturnsError);
	CPPUNIT_TEST(Dir_ReadLink_ReturnsError);
	CPPUNIT_TEST(File_ReadLink_ReturnsError);
	CPPUNIT_TEST(SmallBuffer_ReadLink_ReturnsLength);
	CPPUNIT_TEST(ExtraByteInLength_ReadLink_ResultIsNULLTerminated);
	CPPUNIT_TEST(BadArgs_ReadLink_ReturnsError);

	CPPUNIT_TEST(Uninitialized_MakeLinkedPathDirPath_ReturnsError);
	CPPUNIT_TEST(ExistingAbsDirLink_MakeLinkedPathDirPath_ReturnsOk);
	CPPUNIT_TEST(ExistingAbsFileLink_MakeLinkedPathDirPath_ReturnsOk);
	CPPUNIT_TEST(ExistingAbsCyclicLink_MakeLinkedPathDirPath_ReturnsOk);
	CPPUNIT_TEST(ExistingRelDirLink_MakeLinkedPathDirPath_ReturnsOk);
	CPPUNIT_TEST(ExistingRelFileLink_MakeLinkedPathDirPath_ReturnsOk);
	CPPUNIT_TEST(NullPath_MakeLinkedPathDirPath_ReturnsBadValue);
	CPPUNIT_TEST(NullDirPath_MakeLinkedPathDirPath_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndPath_MakeLinkedPathDirPath_ReturnsBadValue);

	CPPUNIT_TEST(Uninitialized_MakeLinkedPathDirPtr_ReturnsError);
	CPPUNIT_TEST(ExistingAbsDirLink_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(ExistingAbsFileLink_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(ExistingAbsCyclicLink_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(ExistingRelDirLink_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(ExistingRelFileLink_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(AbsLinkUninitializedDir_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(AbsLinkBadDir_MakeLinkedPathDirPtr_ReturnsOk);
	CPPUNIT_TEST(RelLinkUninitializedDir_MakeLinkedPathDirPtr_ReturnsError);
	CPPUNIT_TEST(RelLinkBadDir_MakeLinkedPathDirPtr_ReturnsError);
	CPPUNIT_TEST(NullPath_MakeLinkedPathDirPtr_ReturnsBadValue);
	CPPUNIT_TEST(NullDir_MakeLinkedPathDirPtr_ReturnsBadValue);
	CPPUNIT_TEST(NullDirAndPath_MakeLinkedPathDirPtr_ReturnsBadValue);

	CPPUNIT_TEST(Uninitialized_IsAbsolute_ReturnsFalse);
	CPPUNIT_TEST(ExistingAbsDirLink_IsAbsolute_ReturnsTrue);
	CPPUNIT_TEST(ExistingRelFileLink_IsAbsolute_ReturnsFalse);
	CPPUNIT_TEST(NonExistingLink_IsAbsolute_ReturnsFalse);
	CPPUNIT_TEST(Dir_IsAbsolute_ReturnsFalse);
	CPPUNIT_TEST(File_IsAbsolute_ReturnsFalse);

	CPPUNIT_TEST(Uninitialized_CopyConstructor_ReturnsError);
	CPPUNIT_TEST(ExistingDirLink_CopyConstructor_ReturnsOk);
	CPPUNIT_TEST(ExistingFileLink_CopyConstructor_ReturnsOk);
	CPPUNIT_TEST(Uninitialized1_Assignment_ReturnsError);
	CPPUNIT_TEST(Uninitialized2_Assignment_ReturnsError);
	CPPUNIT_TEST(ExistingDirLink_Assignment_ReturnsOk);
	CPPUNIT_TEST(ExistingFileLink_Assignment_ReturnsOk);
	CPPUNIT_TEST_SUITE_END();

protected:
	void CreateUninitializedNodes(TestNodes& testEntries)
	{
		testEntries.clear();
		testEntries.add(new BSymLink, "");
	}

	void CreateRWNodes(TestNodes& testEntries)
	{
		testEntries.clear();
		const char* filename;
		filename = dirLinkname;
		testEntries.add(new BSymLink(filename), filename);
		filename = fileLinkname;
		testEntries.add(new BSymLink(filename), filename);
	}

	void CreateRONodes(TestNodes& testEntries)
	{
		testEntries.clear();
		const char* filename;
		filename = "/tmp";
		testEntries.add(new BSymLink(filename), filename);
		filename = dirLinkname;
		testEntries.add(new BSymLink(filename), filename);
		filename = fileLinkname;
		testEntries.add(new BSymLink(filename), filename);
		filename = badLinkname;
		testEntries.add(new BSymLink(filename), filename);
		filename = cyclicLinkname1;
		testEntries.add(new BSymLink(filename), filename);
	}

public:
	void setUp() { NodeTest::setUp(); }
	void tearDown() { NodeTest::tearDown(); }

	void Uninitialized_Constructor_ReturnsNoInit()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
	}

	void FileLink_PathConstructor_ReturnsOk()
	{
		BSymLink link(fileLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void NonExisting_PathConstructor_ReturnsEntryNotFound()
	{
		BSymLink link(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Null_PathConstructor_ReturnsBadValue()
	{
		BSymLink link((const char*)NULL);
		CPPUNIT_ASSERT(equals(link.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void Empty_PathConstructor_ReturnsEntryNotFound()
	{
		BSymLink link("");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void ExistingFile_PathConstructor_ReturnsOk()
	{
		BSymLink link(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_PathConstructor_ReturnsOk()
	{
		BSymLink link(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void TooLong_PathConstructor_ReturnsNameTooLong()
	{
		BSymLink link(tooLongEntryname);
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, link.InitCheck());
	}

	void DirectoryLink_EntryConstructor_ReturnsOk()
	{
		BEntry entry(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BSymLink link(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void NonExisting_EntryConstructor_ReturnsEntryNotFound()
	{
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BSymLink link(&entry);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Null_EntryConstructor_ReturnsBadValue()
	{
		BSymLink link((BEntry*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void Uninitialized_EntryConstructor_ReturnsError()
	{
		BEntry entry;
		BSymLink link(&entry);
		CPPUNIT_ASSERT(equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void ExistingFile_EntryConstructor_ReturnsOk()
	{
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BSymLink link(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_EntryConstructor_ReturnsOk()
	{
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BSymLink link(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void TooLong_EntryConstructor_ReturnsError()
	{
		BEntry entry(tooLongEntryname);
		CPPUNIT_ASSERT(equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG));
		BSymLink link(&entry);
		CPPUNIT_ASSERT(equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void DirectoryLink_EntryRefConstructor_ReturnsOk()
	{
		BEntry entry(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BSymLink link(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void NonExisting_EntryRefConstructor_ReturnsEntryNotFound()
	{
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BSymLink link(&ref);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Null_EntryRefConstructor_ReturnsBadValue()
	{
		BSymLink link((entry_ref*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void ExistingFile_EntryRefConstructor_ReturnsOk()
	{
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BSymLink link(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_EntryRefConstructor_ReturnsOk()
	{
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BSymLink link(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void RelativePath_DirAndPathConstructor_ReturnsOk()
	{
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, dirRelLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void InvalidPath_DirAndPathConstructor_ReturnsBadValue()
	{
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void NonExisting_DirAndPathConstructor_ReturnsEntryNotFound()
	{
		BDirectory pathDir(nonExistingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, nonExistingRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void NullArgs_DirAndPathConstructor_ReturnsBadValue()
	{
		BSymLink link((BDirectory*)NULL, (const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void NullDir_DirAndPathConstructor_ReturnsBadValue()
	{
		BSymLink link((BDirectory*)NULL, dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void NullPath_DirAndPathConstructor_ReturnsBadValue()
	{
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, (const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void EmptyPath_DirAndPathConstructor_ReturnsEntryNotFound()
	{
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, "");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void ExistingFile_DirAndPathConstructor_ReturnsOk()
	{
		BDirectory pathDir(existingSuperFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, existingRelFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_DirAndPathConstructor_ReturnsOk()
	{
		BDirectory pathDir(existingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, existingRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void TooLong_DirAndPathConstructor_ReturnsNameTooLong()
	{
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, tooLongRelEntryname);
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, link.InitCheck());
	}

	void FileSuperDir_DirAndPathConstructor_ReturnsEntryNotFound()
	{
		BDirectory pathDir(fileSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BSymLink link(&pathDir, fileRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void FileLink_SetToPath_ReturnsOk()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(fileLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void NonExisting_SetToPath_ReturnsEntryNotFound()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Null_SetToPath_ReturnsBadValue()
	{
		BSymLink link;
		CPPUNIT_ASSERT(equals(link.SetTo((const char*)NULL), B_BAD_VALUE, B_NO_INIT));
		CPPUNIT_ASSERT(equals(link.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void Empty_SetToPath_ReturnsEntryNotFound()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(""));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void ExistingFile_SetToPath_ReturnsOk()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_SetToPath_ReturnsOk()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void TooLong_SetToPath_ReturnsNameTooLong()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, link.SetTo(tooLongEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, link.InitCheck());
	}

	void DirectoryLink_SetToEntry_ReturnsOk()
	{
		BSymLink link;
		BEntry entry(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void NonExisting_SetToEntry_ReturnsEntryNotFound()
	{
		BSymLink link;
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Null_SetToEntry_ReturnsBadValue()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.SetTo((BEntry*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void Uninitialized_SetToEntry_ReturnsError()
	{
		BSymLink link;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		CPPUNIT_ASSERT(equals(link.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void ExistingFile_SetToEntry_ReturnsOk()
	{
		BSymLink link;
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_SetToEntry_ReturnsOk()
	{
		BSymLink link;
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void TooLong_SetToEntry_ReturnsError()
	{
		BSymLink link;
		BEntry entry;
		CPPUNIT_ASSERT(equals(entry.SetTo(tooLongEntryname), E2BIG, B_NAME_TOO_LONG));
		CPPUNIT_ASSERT(equals(link.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(link.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void DirectoryLink_SetToEntryRef_ReturnsOk()
	{
		BSymLink link;
		BEntry entry(dirLinkname);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void NonExisting_SetToEntryRef_ReturnsEntryNotFound()
	{
		BSymLink link;
		BEntry entry(nonExistingDirname);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Null_SetToEntryRef_ReturnsBadValue()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.SetTo((entry_ref*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void ExistingFile_SetToEntryRef_ReturnsOk()
	{
		BSymLink link;
		BEntry entry(existingFilename);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_SetToEntryRef_ReturnsOk()
	{
		BSymLink link;
		BEntry entry(existingDirname);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void RelativePath_SetToDirAndPath_ReturnsOk()
	{
		BSymLink link;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&pathDir, dirRelLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void InvalidPath_SetToDirAndPath_ReturnsBadValue()
	{
		BSymLink link;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.SetTo(&pathDir, dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void NonExisting_SetToDirAndPath_ReturnsEntryNotFound()
	{
		BSymLink link;
		BDirectory pathDir(nonExistingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(&pathDir, nonExistingRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void NullArgs_SetToDirAndPath_ReturnsBadValue()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.SetTo((BDirectory*)NULL, (const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void NullDir_SetToDirAndPath_ReturnsBadValue()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.SetTo((BDirectory*)NULL, dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void NullPath_SetToDirAndPath_ReturnsBadValue()
	{
		BSymLink link;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.SetTo(&pathDir, (const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.InitCheck());
	}

	void EmptyPath_SetToDirAndPath_ReturnsEntryNotFound()
	{
		BSymLink link;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(&pathDir, ""));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void ExistingFile_SetToDirAndPath_ReturnsOk()
	{
		BSymLink link;
		BDirectory pathDir(existingSuperFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&pathDir, existingRelFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void ExistingDir_SetToDirAndPath_ReturnsOk()
	{
		BSymLink link;
		BDirectory pathDir(existingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(&pathDir, existingRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
	}

	void TooLong_SetToDirAndPath_ReturnsNameTooLong()
	{
		BSymLink link;
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, link.SetTo(&pathDir, tooLongRelEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, link.InitCheck());
	}

	void FileSuperDir_SetToDirAndPath_ReturnsEntryNotFound()
	{
		BSymLink link;
		BDirectory pathDir(fileSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(&pathDir, fileRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.InitCheck());
	}

	void Uninitialized_ReadLink_ReturnsError()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		// R5: returns B_BAD_ADDRESS instead of (as doc'ed) B_FILE_ERROR
		CPPUNIT_ASSERT(equals(link.ReadLink(buffer, sizeof(buffer)), B_BAD_ADDRESS, B_FILE_ERROR));
	}

	void ExistingDirLink_ReadLink_ReturnsOk()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		CPPUNIT_ASSERT(link.ReadLink(buffer, sizeof(buffer)) == (ssize_t)strlen(existingDirname));
		CPPUNIT_ASSERT(strcmp(buffer, existingDirname) == 0);
	}

	void ExistingFileLink_ReadLink_ReturnsOk()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(fileLinkname));
		CPPUNIT_ASSERT(link.ReadLink(buffer, sizeof(buffer)) == (ssize_t)strlen(existingFilename));
		CPPUNIT_ASSERT(strcmp(buffer, existingFilename) == 0);
	}

	void ExistingCyclicLink_ReadLink_ReturnsOk()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(cyclicLinkname1));
		CPPUNIT_ASSERT(link.ReadLink(buffer, sizeof(buffer)) == (ssize_t)strlen(cyclicLinkname2));
		CPPUNIT_ASSERT(strcmp(buffer, cyclicLinkname2) == 0);
	}

	void ExistingLinkToNonExisting_ReadLink_ReturnsOk()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(badLinkname));
		CPPUNIT_ASSERT(link.ReadLink(buffer, sizeof(buffer)) == (ssize_t)strlen(nonExistingFilename));
		CPPUNIT_ASSERT(strcmp(buffer, nonExistingFilename) == 0);
	}

	void NonExistingLink_ReadLink_ReturnsError()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(nonExistingFilename));
		// R5: returns B_BAD_ADDRESS instead of (as doc'ed) B_FILE_ERROR
		CPPUNIT_ASSERT(equals(link.ReadLink(buffer, sizeof(buffer)), B_BAD_ADDRESS, B_FILE_ERROR));
	}

	void Dir_ReadLink_ReturnsError()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.ReadLink(buffer, sizeof(buffer)));
	}

	void File_ReadLink_ReturnsError()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.ReadLink(buffer, sizeof(buffer)));
	}

	void SmallBuffer_ReadLink_ReturnsLength()
	{
		BSymLink link;
		// R5: returns the size of the contents, not the number of bytes copied
		// Haiku: ... so do we
		char smallBuffer[2];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		ssize_t linkLength = link.ReadLink(smallBuffer, sizeof(smallBuffer));
		CPPUNIT_ASSERT(linkLength == static_cast<ssize_t>(strlen(existingDirname)));
		CPPUNIT_ASSERT_EQUAL('/', smallBuffer[0]);
		CPPUNIT_ASSERT_EQUAL('\0', smallBuffer[1]);
	}

	void ExtraByteInLength_ReadLink_ResultIsNULLTerminated()
	{
		BSymLink link;
		// Invoke with one extra byte of length to ensure that the result is NULL
		// terminated.
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		buffer[17] = 'x';
		CPPUNIT_ASSERT(link.ReadLink(buffer, 18));
		CPPUNIT_ASSERT_EQUAL('\0', buffer[17]);
		CPPUNIT_ASSERT_EQUAL(strcmp(buffer, "/tmp/existing-dir"), 0);
	}

	void BadArgs_ReadLink_ReturnsError()
	{
		BSymLink link;
		char buffer[B_PATH_NAME_LENGTH + 1];
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(fileLinkname));
		CPPUNIT_ASSERT(equals(link.ReadLink(NULL, sizeof(buffer)), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Uninitialized_MakeLinkedPathDirPath_ReturnsError()
	{
		BSymLink link;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		CPPUNIT_ASSERT(equals(link.MakeLinkedPath("/boot", &path), B_BAD_ADDRESS, B_FILE_ERROR));
	}

	void ExistingAbsDirLink_MakeLinkedPathDirPath_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		CPPUNIT_ASSERT(link.MakeLinkedPath("/boot", &path) == (ssize_t)strlen(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(existingDirname) == path.Path());
	}

	void ExistingAbsFileLink_MakeLinkedPathDirPath_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(fileLinkname));
		CPPUNIT_ASSERT(link.MakeLinkedPath("/boot", &path) == (ssize_t)strlen(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(existingFilename) == path.Path());
	}

	void ExistingAbsCyclicLink_MakeLinkedPathDirPath_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(cyclicLinkname1));
		CPPUNIT_ASSERT(link.MakeLinkedPath("/boot", &path) == (ssize_t)strlen(cyclicLinkname2));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(cyclicLinkname2) == path.Path());
	}

	void ExistingRelDirLink_MakeLinkedPathDirPath_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BEntry entry;
		BPath entryPath;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&entryPath));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relDirLinkname));
		CPPUNIT_ASSERT(link.MakeLinkedPath(existingSuperDirname, &path)
					   == (ssize_t)strlen(entryPath.Path()));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(entryPath == path);
	}

	void ExistingRelFileLink_MakeLinkedPathDirPath_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BEntry entry;
		BPath entryPath;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&entryPath));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relFileLinkname));
		CPPUNIT_ASSERT(link.MakeLinkedPath(existingSuperFilename, &path)
					   == (ssize_t)strlen(entryPath.Path()));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(entryPath == path);
	}

	void NullPath_MakeLinkedPathDirPath_ReturnsBadValue()
	{
#if !TEST_R5
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		// R5: crashs, when passing a NULL path
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.MakeLinkedPath("/boot", NULL));
#endif
	}

	void NullDirPath_MakeLinkedPathDirPath_ReturnsBadValue()
	{
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		BPath path;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.MakeLinkedPath((const char*)NULL, &path));
	}

	void NullDirAndPath_MakeLinkedPathDirPath_ReturnsBadValue()
	{
#if !TEST_R5
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		// R5: crashs, when passing a NULL path
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.MakeLinkedPath((const char*)NULL, NULL));
#endif
	}

	void Uninitialized_MakeLinkedPathDirPtr_ReturnsError()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT(equals(link.MakeLinkedPath(&dir, &path), B_BAD_ADDRESS, B_FILE_ERROR));
	}

	void ExistingAbsDirLink_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(existingDirname) == path.Path());
	}

	void ExistingAbsFileLink_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(fileLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(existingFilename) == path.Path());
	}

	void ExistingAbsCyclicLink_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(cyclicLinkname1));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(cyclicLinkname2));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(cyclicLinkname2) == path.Path());
	}

	void ExistingRelDirLink_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		BEntry entry;
		BPath entryPath;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&entryPath));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relDirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(entryPath.Path()));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(entryPath == path);
	}

	void ExistingRelFileLink_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		BEntry entry;
		BPath entryPath;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&entryPath));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relFileLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(entryPath.Path()));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(entryPath == path);
	}

	void AbsLinkUninitializedDir_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(existingDirname) == path.Path());
	}

	void AbsLinkBadDir_MakeLinkedPathDirPtr_ReturnsOk()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT(link.MakeLinkedPath(&dir, &path) == (ssize_t)strlen(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, path.InitCheck());
		CPPUNIT_ASSERT(std::string(existingDirname) == path.Path());
	}

	void RelLinkUninitializedDir_MakeLinkedPathDirPtr_ReturnsError()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relDirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT(equals(link.MakeLinkedPath(&dir, &path), B_NO_INIT, B_BAD_VALUE));
	}

	void RelLinkBadDir_MakeLinkedPathDirPtr_ReturnsError()
	{
		BSymLink link;
		BPath path;
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relDirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT(equals(link.MakeLinkedPath(&dir, &path), B_NO_INIT, B_BAD_VALUE));
	}

	void NullPath_MakeLinkedPathDirPtr_ReturnsBadValue()
	{
#if !TEST_R5
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		BDirectory dir("/boot");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		// R5: crashs, when passing a NULL path
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, link.MakeLinkedPath(&dir, NULL));
#endif
	}

	void NullDir_MakeLinkedPathDirPtr_ReturnsBadValue()
	{
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		BPath path;
		CPPUNIT_ASSERT(link.MakeLinkedPath((const BDirectory*)NULL, &path) == B_BAD_VALUE);
	}

	void NullDirAndPath_MakeLinkedPathDirPtr_ReturnsBadValue()
	{
#if !TEST_R5
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		// R5: crashs, when passing a NULL path
		CPPUNIT_ASSERT(link.MakeLinkedPath((const BDirectory*)NULL, NULL) == B_BAD_VALUE);
#endif
	}

	void Uninitialized_IsAbsolute_ReturnsFalse()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		CPPUNIT_ASSERT(link.IsAbsolute() == false);
	}

	void ExistingAbsDirLink_IsAbsolute_ReturnsTrue()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(dirLinkname));
		CPPUNIT_ASSERT(link.IsAbsolute() == true);
	}

	void ExistingRelFileLink_IsAbsolute_ReturnsFalse()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(relFileLinkname));
		CPPUNIT_ASSERT(link.IsAbsolute() == false);
	}

	void NonExistingLink_IsAbsolute_ReturnsFalse()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, link.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT(link.IsAbsolute() == false);
	}

	void Dir_IsAbsolute_ReturnsFalse()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(existingDirname));
		CPPUNIT_ASSERT(link.IsAbsolute() == false);
	}

	void File_IsAbsolute_ReturnsFalse()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(existingFilename));
		CPPUNIT_ASSERT(link.IsAbsolute() == false);
	}

	void Uninitialized_CopyConstructor_ReturnsError()
	{
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		BSymLink link2(link);
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(link2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void ExistingDirLink_CopyConstructor_ReturnsOk()
	{
		BSymLink link(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		BSymLink link2(link);
		CPPUNIT_ASSERT_EQUAL(B_OK, link2.InitCheck());
	}

	void ExistingFileLink_CopyConstructor_ReturnsOk()
	{
		BSymLink link(fileLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		BSymLink link2(link);
		CPPUNIT_ASSERT_EQUAL(B_OK, link2.InitCheck());
	}

	void Uninitialized1_Assignment_ReturnsError()
	{
		BSymLink link;
		BSymLink link2;
		link2 = link;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(link2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void Uninitialized2_Assignment_ReturnsError()
	{
		BSymLink link;
		BSymLink link2(dirLinkname);
		link2 = link;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(link2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void ExistingDirLink_Assignment_ReturnsOk()
	{
		BSymLink link(dirLinkname);
		BSymLink link2;
		link2 = link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link2.InitCheck());
	}

	void ExistingFileLink_Assignment_ReturnsOk()
	{
		BSymLink link(fileLinkname);
		BSymLink link2;
		link2 = link;
		CPPUNIT_ASSERT_EQUAL(B_OK, link2.InitCheck());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SymLinkTest, getTestSuiteName());
