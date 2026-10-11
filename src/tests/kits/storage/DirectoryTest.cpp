/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <SymLink.h>


#include "NodeTest.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>



class DirectoryTest : public NodeTest {
	CPPUNIT_TEST_SUB_SUITE(DirectoryTest, NodeTest);
	CPPUNIT_TEST(DefaultConstructor_ReturnsNoInit);

	CPPUNIT_TEST(ExistingDir_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExistingDir_PathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(NullPath_PathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(EmptyPath_PathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(FileAsDir_PathConstructor_ReturnsNotADirectory);
	CPPUNIT_TEST(TooLongPath_PathConstructor_ReturnsNameTooLong);
	CPPUNIT_TEST(FileDirname_PathConstructor_ReturnsNotFound);

	CPPUNIT_TEST(ExistingEntry_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExistingEntry_EntryConstructor_ReturnsNotFound);
	CPPUNIT_TEST(NullEntry_EntryConstructor_ReturnsBadValue);
	CPPUNIT_TEST(UnsetEntry_EntryConstructor_ReturnsBadAddressOrValue);
	CPPUNIT_TEST(FileAsDir_EntryConstructor_ReturnsNotADirectory);
	CPPUNIT_TEST(TooLongPath_EntryConstructor_ReturnsBadAddressOrValue);

	CPPUNIT_TEST(ExistingRef_EntryRefConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExistingRef_EntryRefConstructor_ReturnsNotFound);
	CPPUNIT_TEST(NullRef_EntryRefConstructor_ReturnsBadValue);
	CPPUNIT_TEST(FileAsDir_EntryRefConstructor_ReturnsNotADirectory);

	CPPUNIT_TEST(ExistingRef_NodeRefConstructor_ReturnsOk);
	CPPUNIT_TEST(NullRef_NodeRefConstructor_ReturnsBadValue);
	CPPUNIT_TEST(FileAsDir_NodeRefConstructor_ReturnsNotADirectory);

	CPPUNIT_TEST(ExistingRelDir_DirectoryAndPathConstructor_ReturnsOk);
	CPPUNIT_TEST(AbsoluteDir_DirectoryAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NonExistingSuperRel_DirectoryAndPathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(NullDirAndPath_DirectoryAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullDirExistingPath_DirectoryAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(ExistingDirNullPath_DirectoryAndPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(EmptyPath_DirectoryAndPathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(FileAsDir_DirectoryAndPathConstructor_ReturnsNotADirectory);
	CPPUNIT_TEST(TooLongPath_DirectoryAndPathConstructor_ReturnsNameTooLong);
	CPPUNIT_TEST(FileDirname_DirectoryAndPathConstructor_ReturnsNotFound);

	CPPUNIT_TEST(ExistingDir_SetToPath_ReturnsOk);
	CPPUNIT_TEST(NonExistingDir_SetToPath_ReturnsNotFound);
	CPPUNIT_TEST(NullPath_SetToPath_ReturnsBadValue);
	CPPUNIT_TEST(EmptyPath_SetToPath_ReturnsNotFound);
	CPPUNIT_TEST(FileAsDir_SetToPath_ReturnsNotADirectory);
	CPPUNIT_TEST(TooLongPath_SetToPath_ReturnsNameTooLong);
	CPPUNIT_TEST(FileDirname_SetToPath_ReturnsNotFound);

	CPPUNIT_TEST(ExistingEntry_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(NonExistingEntry_SetToEntry_ReturnsNotFound);
	CPPUNIT_TEST(NullEntry_SetToEntry_ReturnsBadValue);
	CPPUNIT_TEST(UnsetEntry_SetToEntry_ReturnsBadAddressOrValue);
	CPPUNIT_TEST(FileAsDir_SetToEntry_ReturnsNotADirectory);
	CPPUNIT_TEST(TooLongPath_SetToEntry_ReturnsBadAddressOrValue);

	CPPUNIT_TEST(ExistingRef_SetToEntryRef_ReturnsOk);
	CPPUNIT_TEST(NonExistingRef_SetToEntryRef_ReturnsNotFound);
	CPPUNIT_TEST(NullRef_SetToEntryRef_ReturnsBadValue);
	CPPUNIT_TEST(FileAsDir_SetToEntryRef_ReturnsNotADirectory);

	CPPUNIT_TEST(ExistingRef_SetToNodeRef_ReturnsOk);
	CPPUNIT_TEST(NullRef_SetToNodeRef_ReturnsBadValue);
	CPPUNIT_TEST(FileAsDir_SetToNodeRef_ReturnsNotADirectory);

	CPPUNIT_TEST(ExistingRelDir_SetToDirectoryAndPath_ReturnsOk);
	CPPUNIT_TEST(AbsoluteDir_SetToDirectoryAndPath_ReturnsBadValue);
	CPPUNIT_TEST(NonExistingSuperRel_SetToDirectoryAndPath_ReturnsNotFound);
	CPPUNIT_TEST(NullDirAndPath_SetToDirectoryAndPath_ReturnsBadValue);
	CPPUNIT_TEST(NullDirExistingPath_SetToDirectoryAndPath_ReturnsBadValue);
	CPPUNIT_TEST(ExistingDirNullPath_SetToDirectoryAndPath_ReturnsBadValue);
	CPPUNIT_TEST(EmptyPath_SetToDirectoryAndPath_ReturnsNotFound);
	CPPUNIT_TEST(FileAsDir_SetToDirectoryAndPath_ReturnsNotADirectory);
	CPPUNIT_TEST(TooLongPath_SetToDirectoryAndPath_ReturnsNameTooLong);
	CPPUNIT_TEST(FileDirname_SetToDirectoryAndPath_ReturnsNotFound);

	CPPUNIT_TEST(UninitializedDir_GetEntry_ReturnsNoInit);
	CPPUNIT_TEST(ExistingDir_GetEntry_ReturnsOk);
	CPPUNIT_TEST(NullEntry_GetEntry_ReturnsBadValue);
	CPPUNIT_TEST(NonExistingDir_GetEntry_ReturnsNoInit);

	CPPUNIT_TEST(UninitializedDir_IsRoot_ReturnsFalse);
	CPPUNIT_TEST(RootDir_IsRoot_ReturnsTrue);
	CPPUNIT_TEST(SystemDir_IsRoot_ReturnsTrue);
	CPPUNIT_TEST(TmpDir_IsRoot_ReturnsFalse);
	CPPUNIT_TEST(Root_IsRoot_ReturnsTrue);

	CPPUNIT_TEST(ExistingPathUninitializedDir_FindEntry_ReturnsOk);
	CPPUNIT_TEST(ExistingPathBadlyInitializedDir_FindEntry_ReturnsOk);
	CPPUNIT_TEST(ExistingRelPathUninitializedDir_FindEntry_ReturnsOk);
	CPPUNIT_TEST(ExistingRelDirectoryMismatch_FindEntry_ReturnsNotFound);
	CPPUNIT_TEST(AbstractEntry_FindEntry_ReturnsNotFound);
	CPPUNIT_TEST(NullEntry_FindEntry_ReturnsBadValue);
	CPPUNIT_TEST(NullName_FindEntry_ReturnsBadValue);
	CPPUNIT_TEST(DontTraverseValidLink_FindEntry_ReturnsLinkEntry);
	CPPUNIT_TEST(TraverseValidLink_FindEntry_ReturnsTargetEntry);
	CPPUNIT_TEST(DontTraverseInvalidLink_FindEntry_ReturnsLinkEntry);
	CPPUNIT_TEST(TraverseInvalidLink_FindEntry_ReturnsNotFound);
	CPPUNIT_TEST(DontTraverseCyclicLink_FindEntry_ReturnsLinkEntry);
	CPPUNIT_TEST(TraverseCyclicLink_FindEntry_ReturnsLinkLimit);

	CPPUNIT_TEST(ExistingDir_ContainsExistingPath_ReturnsTrue);
	CPPUNIT_TEST(ExistingDir_ContainsNonExistingPath_ReturnsFalse);
	CPPUNIT_TEST(ExistingDir_ContainsNullPath_ReturnsTrue);
	CPPUNIT_TEST(UninitializedDir_ContainsExistingPath_ReturnsFalse);
	CPPUNIT_TEST(UninitializedDir_ContainsNullPath_ReturnsFalse);
	CPPUNIT_TEST(UninitializedDir_ContainsNonExistingPath_ReturnsFalse);
	CPPUNIT_TEST(BadlyInitializedDir_ContainsExistingPath_ReturnsFalse);
	CPPUNIT_TEST(BadlyInitializedDir_ContainsNonExistingPath_ReturnsFalse);
	CPPUNIT_TEST(ExistingDir_ContainsSecondLevelAbsolutePath_ReturnsTrue);
	CPPUNIT_TEST(ExistingDir_ContainsSecondLevelRelativePath_ReturnsFalse);
	CPPUNIT_TEST(ExistingEntry_ContainsSelf_ReturnsTrue);
	CPPUNIT_TEST(AbsoluteDirPathMatchingDirectoryKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(AbsoluteDirPathMismatchingFileKind_Contains_ReturnsFalse);
	CPPUNIT_TEST(AbsoluteFilePathMatchingFileKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(AbsoluteFilePathMismatchingSymlinkKind_Contains_ReturnsFalse);
	CPPUNIT_TEST(DirectoryLinkPathMatchingSymlinkKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(DirectoryLinkPathMismatchingDirectoryKind_Contains_ReturnsFalse);
	CPPUNIT_TEST(RelativeFilePathMatchingFileKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(RelativePathMismatchingSymlinkKind_Contains_ReturnsFalse);
	CPPUNIT_TEST(ExistingEntry_Contains_ReturnsTrue);
	CPPUNIT_TEST(UninitializedDir_ContainsExistingEntry_ReturnsFalse);
	CPPUNIT_TEST(UninitializedDir_ContainsNonExistingEntry_ReturnsFalse);
	CPPUNIT_TEST(BadlyInitializedDir_ContainsExistingEntry_ReturnsFalse);
	CPPUNIT_TEST(BadlyInitializedDir_ContainsNonExistingEntry_ReturnsFalse);
	CPPUNIT_TEST(ExistingDir_ContainsNonExistingEntry_ReturnsFalse);
	CPPUNIT_TEST(ExistingDir_ContainsNullEntry_ReturnsFalse);
	CPPUNIT_TEST(UninitializedDir_ContainsNullEntry_ReturnsFalse);
	CPPUNIT_TEST(ExistingDir_ContainsSecondLevelExistingDirEntry_ReturnsTrue);
	CPPUNIT_TEST(ExistingDir_ContainsSelf_ReturnsTrueOrFalse);
	CPPUNIT_TEST(DirMatchingKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(DirMismatchingKind_Contains_ReturnsFalse);
	CPPUNIT_TEST(FileMatchingKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(FileMismatchingKind_Contains_ReturnsFalse);
	CPPUNIT_TEST(LinkMatchingKind_Contains_ReturnsTrue);
	CPPUNIT_TEST(LinkMismatchingKind_Contains_ReturnsFalse);

	CPPUNIT_TEST(UninitializedDir_GetStatFor_ReturnsNoInit);
	CPPUNIT_TEST(BadlyInitializedDir_GetStatFor_ReturnsNoInit);
	CPPUNIT_TEST(InitializedDirAbsolute_GetStatFor_ReturnsOk);
	CPPUNIT_TEST(InitializedDirRelative_GetStatFor_ReturnsOk);
	CPPUNIT_TEST(NonExistingEntry_GetStatFor_ReturnsNotFound);

	CPPUNIT_TEST(NullPath_GetStatFor_ReturnsOk);
	CPPUNIT_TEST(EmptyPath_GetStatFor_ReturnsNotFound);
	CPPUNIT_TEST(NullStatAndPath_GetStatFor_ReturnsBadValue);
	CPPUNIT_TEST(NullStat_GetStatFor_ReturnsBadValue);

	CPPUNIT_TEST(EmptyDir_GetNextEntry_ReturnsNotFound);
	CPPUNIT_TEST(NonEmptyDir_GetNextEntry_ReturnsEntries);
	CPPUNIT_TEST(EmptyDir_GetNextRef_ReturnsNotFound);
	CPPUNIT_TEST(NonEmptyDir_GetNextRef_ReturnsEntries);
	CPPUNIT_TEST(EmptyDir_GetNextDirents_ReturnsOk);
	CPPUNIT_TEST(NonEmptyDir_GetNextDirents_ReturnsEntries);
	CPPUNIT_TEST(NonEmptyDir_GetNextEntryAndRewind_ResetsIteration);
	CPPUNIT_TEST(NonEmptyDir_CountEntries_ReturnsCorrectCount);
	CPPUNIT_TEST(NonEmptyDir_NextEntryRefDirentsInterleaved_ReturnsEntries);

	CPPUNIT_TEST(EntryIteration_Uninitialized_ReturnsError);
	CPPUNIT_TEST(EntryIteration_BadlyInitialized_ReturnsError);
	CPPUNIT_TEST(EntryIteration_BadArgs_ReturnsError);
	CPPUNIT_TEST(NonEmptyDirMixedEntries_GetNextEntry_ReturnsAll);
	CPPUNIT_TEST(NonEmptyDirMixedEntries_GetNextRef_ReturnsAll);
	CPPUNIT_TEST(NonEmptyDirMixedEntries_GetNextDirents_ReturnsAll);
	CPPUNIT_TEST(NonEmptyDirMixedEntries_CountEntries_ReturnsAll);
	CPPUNIT_TEST(NonEmptyDirMixedEntries_GetNextEntryRefDirents_ReturnsAll);
	CPPUNIT_TEST(DirWithLinks_GetNextEntryWithTraverseLinks_ReturnsTarget);

	CPPUNIT_TEST(RelativePath_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(RelativePath_CreateFile_ReturnsOk);
	CPPUNIT_TEST(RelativePath_CreateFileWithFailIfExistsFalse_ReturnsOk);
	CPPUNIT_TEST(RelativePath_CreateSymLink_ReturnsOk);
	CPPUNIT_TEST(AbsolutePath_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(AbsolutePath_CreateFile_ReturnsOk);
	CPPUNIT_TEST(AbsolutePath_CreateFileWithFailIfExistsFalse_ReturnsOk);
	CPPUNIT_TEST(AbsolutePath_CreateSymLink_ReturnsOk);
	CPPUNIT_TEST(ExistingEntries_CreateDirectory_ReturnsFileExists);
	CPPUNIT_TEST(ExistingEntries_CreateFile_ReturnsFileExists);
	CPPUNIT_TEST(ExistingEntries_CreateFileWithFailIfExistsFalse_ReturnsOk);
	CPPUNIT_TEST(ExistingEntries_CreateSymLink_ReturnsFileExists);

	CPPUNIT_TEST(UninitializedDirAndNonExistingEntry_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(UninitializedDirAndExistingEntry_CreateDirectory_ReturnsFileExists);
	CPPUNIT_TEST(UninitializedDirAndNonExistingEntry_CreateFile_ReturnsOk);
	CPPUNIT_TEST(UninitializedDirAndExistingEntry_CreateFile_ReturnsFileExists);
	CPPUNIT_TEST(UninitializedDirAndExistingEntry_CreateFileWithFailIfExistsFalse_ReturnsOk);
	CPPUNIT_TEST(UninitializedDirAndNonExistingEntry_CreateSymLink_ReturnsOk);
	CPPUNIT_TEST(UninitializedDirAndExistingEntry_CreateSymLink_ReturnsFileExists);
	CPPUNIT_TEST(RelativePathUninitializedDirAndNonExistingEntry_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(RelativePathUninitializedDirAndExistingEntry_CreateDirectory_ReturnsFileExists);
	CPPUNIT_TEST(RelativePathUninitializedDirAndNonExistingEntry_CreateFile_ReturnsOk);
	CPPUNIT_TEST(RelativePathUninitializedDirAndExistingEntry_CreateFile_ReturnsFileExists);
	CPPUNIT_TEST(RelativePathUninitializedDirAndExistingEntry_CreateFileWithFailIfExistsFalse_ReturnsOk);
	CPPUNIT_TEST(RelativePathUninitializedDirAndNonExistingEntry_CreateSymLink_ReturnsOk);
	CPPUNIT_TEST(RelativePathUninitializedDirAndExistingEntry_CreateSymLink_ReturnsFileExists);
	CPPUNIT_TEST(NullEntryName_CreateDirectory_ReturnsBadValue);
	CPPUNIT_TEST(NullEntryName_CreateFile_ReturnsError);
	CPPUNIT_TEST(NullArgs_CreateSymLink_ReturnsBadValue);
	CPPUNIT_TEST(NullOutParamAndNonExistingEntry_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(NullOutParamAndExistingEntry_CreateDirectory_ReturnsFileExists);
	CPPUNIT_TEST(NullOutParamAndNonExistingEntry_CreateFile_ReturnsOk);
	CPPUNIT_TEST(NullOutParamAndExistingEntry_CreateFile_ReturnsFileExists);
	CPPUNIT_TEST(NullOutParamAndExistingEntry_CreateFileWithFailIfExistsFalse_ReturnsOk);
	CPPUNIT_TEST(NullOutParamAndNonExistingEntry_CreateSymLink_ReturnsOk);
	CPPUNIT_TEST(NullOutParamAndExistingEntry_CreateSymLink_ReturnsFileExists);

	CPPUNIT_TEST(UninitializedDir_CopyConstructor_ReturnsNoInit);
	CPPUNIT_TEST(ExistingDir_CopyConstructor_ReturnsOk);
	CPPUNIT_TEST(UninitializedToUninitialized_Assignment_ReturnsNoInit);
	CPPUNIT_TEST(UninitializedToExisting_Assignment_ReturnsNoInit);
	CPPUNIT_TEST(Assignment_Operator_AssignsCorrectly);

	CPPUNIT_TEST(AbsolutePath_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(RelativePath_CreateDirectory_ReturnsOk);
	CPPUNIT_TEST(AbsolutePathAndNestedPath_CreateDirectoryCFunc_ReturnsOk);
	CPPUNIT_TEST(AbsolutePathAndSingleLevel_CreateDirectoryCFunc_ReturnsOk);
	CPPUNIT_TEST(AbsolutePathAndExistingDir_CreateDirectoryCFunc_ReturnsOk);
	CPPUNIT_TEST(RelativePathAndNestedPath_CreateDirectoryCFunc_ReturnsOk);
	CPPUNIT_TEST(RelativePathAndSingleLevel_CreateDirectoryCFunc_ReturnsOk);
	CPPUNIT_TEST(RelativePathAndExistingDir_CreateDirectoryCFunc_ReturnsOk);
	CPPUNIT_TEST(ExistingFileOrLink_CreateDirectoryCFunc_ReturnsExpected);
	CPPUNIT_TEST(NullPath_CreateDirectoryCFunc_ReturnsBadValue);
	CPPUNIT_TEST_SUITE_END();

	const char* _CreateMultifileDirStructure(TestSet &testSet)
	{
		// iteration over a directory containing files, dirs and symlinks
		const char* iterDir = "/tmp/test-iter-dir";
		create_directory(iterDir, S_IRWXU);
		std::string dirPathName(std::string (iterDir) + "/");
		std::string entryName = "file1";
		creat((dirPathName + entryName).c_str(), S_IRWXU);
		testSet.add(entryName);
		entryName = "file2";
		creat((dirPathName + entryName).c_str(), S_IRWXU);
		testSet.add(entryName);
		entryName = "file3";
		creat((dirPathName + entryName).c_str(), S_IRWXU);
		testSet.add(entryName);
		entryName = "dir1";
		create_directory((dirPathName + entryName).c_str(), S_IRWXU);
		testSet.add(entryName);
		entryName = "dir2";
		create_directory((dirPathName + entryName).c_str(), S_IRWXU);
		testSet.add(entryName);
		entryName = "dir3";
		create_directory((dirPathName + entryName).c_str(), S_IRWXU);
		testSet.add(entryName);
		entryName = "link1";
		symlink(existingFilename, (dirPathName + entryName).c_str());
		testSet.add(entryName);
		entryName = "link2";
		symlink(existingFilename, (dirPathName + entryName).c_str());
		testSet.add(entryName);
		entryName = "link3";
		symlink(existingFilename, (dirPathName + entryName).c_str());
		testSet.add(entryName);
	}

protected:
	void CreateUninitializedNodes(TestNodes& testEntries)
	{
		testEntries.clear();
		testEntries.add(new BDirectory, "");
	}

	void CreateRWNodes(TestNodes& testEntries)
	{
		testEntries.clear();
		const char* filename;
		filename = existingDirname;
		testEntries.add(new BDirectory(filename), filename);
		filename = existingSubDirname;
		testEntries.add(new BDirectory(filename), filename);
	}

	void CreateRONodes(TestNodes& testEntries)
	{
		testEntries.clear();
		const char* filename;
		filename = "/";
		testEntries.add(new BDirectory(filename), filename);
		filename = "/boot";
		testEntries.add(new BDirectory(filename), filename);
		filename = "/boot/home";
		testEntries.add(new BDirectory(filename), filename);
		filename = existingDirname;
		testEntries.add(new BDirectory(filename), filename);
	}

public:
	void setUp() { NodeTest::setUp(); }

	void tearDown() { NodeTest::tearDown(); }

	void DefaultConstructor_ReturnsNoInit()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
	}

	void ExistingDir_PathConstructor_ReturnsOk()
	{
		BDirectory dir(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NonExistingDir_PathConstructor_ReturnsNotFound()
	{
		BDirectory dir(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullPath_PathConstructor_ReturnsBadValue()
	{
		BDirectory dir((const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void EmptyPath_PathConstructor_ReturnsNotFound()
	{
		BDirectory dir("");
		// BeOS R5 returns B_ENTRY_NOT_FOUND instead of B_BAD_VALUE.
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void FileAsDir_PathConstructor_ReturnsNotADirectory()
	{
		BDirectory dir(existingFilename);
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(dir.InitCheck(), B_NOT_A_DIRECTORY);
	}

	void TooLongPath_PathConstructor_ReturnsNameTooLong()
	{
		BDirectory dir(tooLongEntryname);
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, dir.InitCheck());
	}

	void FileDirname_PathConstructor_ReturnsNotFound()
	{
		BDirectory dir(fileDirname);
		// BeOS R5 returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void ExistingEntry_EntryConstructor_ReturnsOk()
	{
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BDirectory dir(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NonExistingEntry_EntryConstructor_ReturnsNotFound()
	{
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BDirectory dir(&entry);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullEntry_EntryConstructor_ReturnsBadValue()
	{
		BDirectory dir((BEntry*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void UnsetEntry_EntryConstructor_ReturnsBadAddressOrValue()
	{
		BEntry entry;
		BDirectory dir(&entry);
		CPPUNIT_ASSERT(equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void FileAsDir_EntryConstructor_ReturnsNotADirectory()
	{
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BDirectory dir(&entry);
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(dir.InitCheck(), B_NOT_A_DIRECTORY);
	}

	void TooLongPath_EntryConstructor_ReturnsBadAddressOrValue()
	{
		BEntry entry(tooLongEntryname);
		// BeOS R5 returns E2BIG instead of B_NAME_TOO_LONG
		CPPUNIT_ASSERT(equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG));
		BDirectory dir(&entry);
		CPPUNIT_ASSERT(equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void ExistingRef_EntryRefConstructor_ReturnsOk()
	{
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BDirectory dir(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NonExistingRef_EntryRefConstructor_ReturnsNotFound()
	{
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BDirectory dir(&ref);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullRef_EntryRefConstructor_ReturnsBadValue()
	{
		BDirectory dir((entry_ref*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void FileAsDir_EntryRefConstructor_ReturnsNotADirectory()
	{
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BDirectory dir(&ref);
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void ExistingRef_NodeRefConstructor_ReturnsOk()
	{
		BNode node(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
		node_ref nref;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.GetNodeRef(&nref));
		BDirectory dir(&nref);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NullRef_NodeRefConstructor_ReturnsBadValue()
	{
		BDirectory dir((node_ref*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void FileAsDir_NodeRefConstructor_ReturnsNotADirectory()
	{
		BNode node(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
		node_ref nref;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.GetNodeRef(&nref));
		BDirectory dir(&nref);
		// BeOS R5: returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void ExistingRelDir_DirectoryAndPathConstructor_ReturnsOk()
	{
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, existingRelSubDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void AbsoluteDir_DirectoryAndPathConstructor_ReturnsBadValue()
	{
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, existingSubDirname);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void NonExistingSuperRel_DirectoryAndPathConstructor_ReturnsNotFound()
	{
		BDirectory Directory(nonExistingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, nonExistingRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullDirAndPath_DirectoryAndPathConstructor_ReturnsBadValue()
	{
		BDirectory dir((BDirectory*)NULL, (const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void NullDirExistingPath_DirectoryAndPathConstructor_ReturnsBadValue()
	{
		BDirectory dir((BDirectory*)NULL, existingSubDirname);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void ExistingDirNullPath_DirectoryAndPathConstructor_ReturnsBadValue()
	{
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, (const char*)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void EmptyPath_DirectoryAndPathConstructor_ReturnsNotFound()
	{
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, "");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void FileAsDir_DirectoryAndPathConstructor_ReturnsNotADirectory()
	{
		BDirectory Directory(existingSuperFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, existingRelFilename);
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void TooLongPath_DirectoryAndPathConstructor_ReturnsNameTooLong()
	{
		BDirectory Directory(tooLongSuperEntryname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, tooLongRelEntryname);
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, dir.InitCheck());
	}

	void FileDirname_DirectoryAndPathConstructor_ReturnsNotFound()
	{
		BDirectory Directory(fileSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		BDirectory dir(&Directory, fileRelDirname);
		// BeOS R5 returns B_ENTRY_NOT_FOUND instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void ExistingDir_SetToPath_ReturnsOk()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NonExistingDir_SetToPath_ReturnsNotFound()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullPath_SetToPath_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo((const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void EmptyPath_SetToPath_ReturnsNotFound()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(""));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void FileAsDir_SetToPath_ReturnsNotADirectory()
	{
		BDirectory dir;
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void TooLongPath_SetToPath_ReturnsNameTooLong()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, dir.SetTo(tooLongEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, dir.InitCheck());
	}

	void FileDirname_SetToPath_ReturnsNotFound()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(fileDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void ExistingEntry_SetToEntry_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NonExistingEntry_SetToEntry_ReturnsNotFound()
	{
		BDirectory dir;
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullEntry_SetToEntry_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo((BEntry*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void UnsetEntry_SetToEntry_ReturnsBadAddressOrValue()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT(equals(dir.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void FileAsDir_SetToEntry_ReturnsNotADirectory()
	{
		BDirectory dir;
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void TooLongPath_SetToEntry_ReturnsBadAddressOrValue()
	{
		BDirectory dir;
		BEntry entry(tooLongEntryname);
		// BeOS R5 returns E2BIG instead of B_NAME_TOO_LONG
		CPPUNIT_ASSERT(equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG));
		CPPUNIT_ASSERT(equals(dir.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(dir.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void ExistingRef_SetToEntryRef_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NonExistingRef_SetToEntryRef_ReturnsNotFound()
	{
		BDirectory dir;
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullRef_SetToEntryRef_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo((entry_ref*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void FileAsDir_SetToEntryRef_ReturnsNotADirectory()
	{
		BDirectory dir;
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void ExistingRef_SetToNodeRef_ReturnsOk()
	{
		BDirectory dir;
		BNode node(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
		node_ref nref;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.GetNodeRef(&nref));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(&nref));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void NullRef_SetToNodeRef_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo((node_ref*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void FileAsDir_SetToNodeRef_ReturnsNotADirectory()
	{
		BDirectory dir;
		BNode node(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
		node_ref nref;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.GetNodeRef(&nref));
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.SetTo(&nref));
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void ExistingRelDir_SetToDirectoryAndPath_ReturnsOk()
	{
		BDirectory dir;
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(&Directory, existingRelSubDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
	}

	void AbsoluteDir_SetToDirectoryAndPath_ReturnsBadValue()
	{
		BDirectory dir;
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo(&Directory, existingSubDirname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void NonExistingSuperRel_SetToDirectoryAndPath_ReturnsNotFound()
	{
		BDirectory dir;
		BDirectory Directory(nonExistingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(&Directory, nonExistingRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void NullDirAndPath_SetToDirectoryAndPath_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo((BDirectory*)NULL, (const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void NullDirExistingPath_SetToDirectoryAndPath_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo((BDirectory*)NULL, existingSubDirname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void ExistingDirNullPath_SetToDirectoryAndPath_ReturnsBadValue()
	{
		BDirectory dir;
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.SetTo(&Directory, (const char*)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.InitCheck());
	}

	void EmptyPath_SetToDirectoryAndPath_ReturnsNotFound()
	{
		BDirectory dir;
		BDirectory Directory(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		// BeOS R5 initializes dir to Directory instead of B_ENTRY_NOT_FOUND
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(&Directory, ""));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void FileAsDir_SetToDirectoryAndPath_ReturnsNotADirectory()
	{
		BDirectory dir;
		BDirectory Directory(existingSuperFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		// BeOS R5 returns B_BAD_VALUE instead of B_NOT_A_DIRECTORY.
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.SetTo(&Directory, existingRelFilename));
		CPPUNIT_ASSERT_EQUAL(B_NOT_A_DIRECTORY, dir.InitCheck());
	}

	void TooLongPath_SetToDirectoryAndPath_ReturnsNameTooLong()
	{
		BDirectory dir;
		BDirectory Directory(tooLongSuperEntryname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, dir.SetTo(&Directory, tooLongRelEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, dir.InitCheck());
	}

	void FileDirname_SetToDirectoryAndPath_ReturnsNotFound()
	{
		BDirectory dir;
		BDirectory Directory(fileSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, Directory.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(&Directory, fileRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
	}

	void UninitializedDir_GetEntry_ReturnsNoInit()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.GetEntry(&entry));
	}

	void ExistingDir_GetEntry_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetEntry(&entry));
		CPPUNIT_ASSERT(entry == BEntry(existingDirname));
	}

	void NullEntry_GetEntry_ReturnsBadValue()
	{
#if !TEST_R5
		// R5: crashs, when passing a NULL BEntry.
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.GetEntry((BEntry*)NULL));
#endif
	}

	void NonExistingDir_GetEntry_ReturnsNoInit()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.GetEntry(&entry));
	}

	void UninitializedDir_IsRoot_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT(dir.IsRootDirectory() == false);
	}

	void RootDir_IsRoot_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.IsRootDirectory() == true);
	}

	void SystemDir_IsRoot_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/boot/system"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(true, dir.IsRootDirectory());
	}

	void TmpDir_IsRoot_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/tmp"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.IsRootDirectory() == false);
	}

	void Root_IsRoot_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.IsRootDirectory() == true);
	}

	void ExistingPathUninitializedDir_FindEntry_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedExistingPath(existingDirname, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedExistingPath.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(existingDirname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != existingDirname);
		CPPUNIT_ASSERT(path == normalizedExistingPath);
	}

	void ExistingPathBadlyInitializedDir_FindEntry_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedExistingPath(existingDirname, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedExistingPath.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(existingDirname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != existingDirname);
		CPPUNIT_ASSERT(path == normalizedExistingPath);
	}

	void ExistingRelPathUninitializedDir_FindEntry_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedExistingSubPath(existingSubDirname, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedExistingSubPath.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		chdir(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(existingRelSubDirname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != existingSubDirname);
		CPPUNIT_ASSERT(path == normalizedExistingSubPath);
		chdir("/");
	}

	void ExistingRelDirectoryMismatch_FindEntry_ReturnsNotFound()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSubDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		chdir(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.FindEntry(existingRelSubDirname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		chdir("/");
	}

	void AbstractEntry_FindEntry_ReturnsNotFound()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(nonExistingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		chdir(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.FindEntry(nonExistingRelDirname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());
		chdir("/");
	}

	void NullEntry_FindEntry_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.FindEntry(existingRelSubDirname, NULL));
	}

	void NullName_FindEntry_ReturnsBadValue()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.FindEntry(NULL, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.FindEntry(NULL, NULL));
	}

	void DontTraverseValidLink_FindEntry_ReturnsLinkEntry()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedDirLinkPath(dirLinkname, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedDirLinkPath.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(dirLinkname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != dirLinkname);
		CPPUNIT_ASSERT(path == normalizedDirLinkPath);
	}

	void TraverseValidLink_FindEntry_ReturnsTargetEntry()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedExistingPath(existingDirname, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedExistingPath.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(dirLinkname, &entry, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != existingDirname);
		CPPUNIT_ASSERT(path == normalizedExistingPath);
	}

	void DontTraverseInvalidLink_FindEntry_ReturnsLinkEntry()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedBadLinkPath(badLinkname, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedBadLinkPath.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(badLinkname, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != badLinkname);
		CPPUNIT_ASSERT(path == normalizedBadLinkPath);
	}

	void TraverseInvalidLink_FindEntry_ReturnsNotFound()
	{
		BDirectory dir;
		BEntry entry;

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.FindEntry(badLinkname, &entry, true));
		CPPUNIT_ASSERT(equals(entry.InitCheck(), B_ENTRY_NOT_FOUND, B_NO_INIT));
	}

	void DontTraverseCyclicLink_FindEntry_ReturnsLinkEntry()
	{
		BDirectory dir;
		BEntry entry;
		BPath path;
		BPath normalizedCyclicLink1(cyclicLinkname1, NULL, true);
		CPPUNIT_ASSERT_EQUAL(normalizedCyclicLink1.InitCheck(), B_OK);

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.FindEntry(cyclicLinkname1, &entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
		CPPUNIT_ASSERT(path != cyclicLinkname1);
		CPPUNIT_ASSERT(path == normalizedCyclicLink1);
	}

	void TraverseCyclicLink_FindEntry_ReturnsLinkLimit()
	{
		BDirectory dir;
		BEntry entry;

		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_LINK_LIMIT, dir.FindEntry(cyclicLinkname1, &entry, true));
		CPPUNIT_ASSERT_EQUAL(B_LINK_LIMIT, entry.InitCheck());
	}

	void ExistingDir_ContainsExistingPath_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingFilename) == true);
	}

	void ExistingDir_ContainsNonExistingPath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(nonExistingFilename) == false);
	}

	void ExistingDir_ContainsNullPath_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains((const char*)NULL) == true);
	}

	void UninitializedDir_ContainsExistingPath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
#if TEST_R5
		CPPUNIT_ASSERT(dir.Contains(existingDirname) == true);
#else
		CPPUNIT_ASSERT(dir.Contains(existingDirname) == false);
#endif
	}

	void UninitializedDir_ContainsNullPath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains((const char*)NULL) == false);
	}

	void UninitializedDir_ContainsNonExistingPath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(nonExistingDirname) == false);
	}

	void BadlyInitializedDir_ContainsExistingPath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
#if TEST_R5
		CPPUNIT_ASSERT(dir.Contains(existingDirname) == true);
#else
		CPPUNIT_ASSERT(dir.Contains(existingDirname) == false);
#endif
	}

	void BadlyInitializedDir_ContainsNonExistingPath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		// non-existing entry, badly initialized BDirectory
		CPPUNIT_ASSERT(dir.Contains(nonExistingDirname) == false);
	}

	void ExistingDir_ContainsSecondLevelAbsolutePath_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingSubDirname) == true);
	}

	void ExistingDir_ContainsSecondLevelRelativePath_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingRelSubDirname) == false);
	}

	void ExistingEntry_ContainsSelf_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingDirname) == true);
	}

	void AbsoluteDirPathMatchingDirectoryKind_Contains_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingDirname, B_DIRECTORY_NODE) == true);
	}

	void AbsoluteDirPathMismatchingFileKind_Contains_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingDirname, B_FILE_NODE) == false);
	}

	void AbsoluteFilePathMatchingFileKind_Contains_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingFilename, B_FILE_NODE) == true);
	}

	void AbsoluteFilePathMismatchingSymlinkKind_Contains_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingFilename, B_SYMLINK_NODE) == false);
	}

	void DirectoryLinkPathMatchingSymlinkKind_Contains_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(dirLinkname, B_SYMLINK_NODE) == true);
	}

	void DirectoryLinkPathMismatchingDirectoryKind_Contains_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(dirLinkname, B_DIRECTORY_NODE) == false);
	}

	void RelativeFilePathMatchingFileKind_Contains_ReturnsTrue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingRelFilename, B_FILE_NODE) == true);
	}

	void RelativePathMismatchingSymlinkKind_Contains_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains(existingRelFilename, B_SYMLINK_NODE) == false);
	}

	void ExistingEntry_Contains_ReturnsTrue()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingSubDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry) == true);
	}

	void UninitializedDir_ContainsExistingEntry_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		// existing entry, uninitialized BDirectory
		// R5: unlike the other version, this one returns false
		// Haiku: both versions return false
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry) == false);
	}

	void UninitializedDir_ContainsNonExistingEntry_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry) == false);
	}

	void BadlyInitializedDir_ContainsExistingEntry_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		// existing entry, badly initialized BDirectory
		// R5: unlike the other version, this one returns false
		// Haiku: both versions return false
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry) == false);
	}

	void BadlyInitializedDir_ContainsNonExistingEntry_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry) == false);
	}

	void ExistingDir_ContainsNonExistingEntry_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(nonExistingFilename));
		CPPUNIT_ASSERT(dir.Contains(&entry) == false);
	}

	void ExistingDir_ContainsNullEntry_ReturnsFalse()
	{
		// R5 crashs, when passing a NULL BEntry
#if !TEST_R5
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains((const BEntry*)NULL) == false);
#endif
	}

	void UninitializedDir_ContainsNullEntry_ReturnsFalse()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT(dir.Contains((const BEntry*)NULL) == false);
	}

	void ExistingDir_ContainsSecondLevelExistingDirEntry_ReturnsTrue()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingSubDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry) == true);
	}

	void ExistingDir_ContainsSelf_ReturnsTrueOrFalse()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		// R5: behavior is different from Contains(const char*)
		// Haiku: both versions return true
		CPPUNIT_ASSERT(equals(dir.Contains(&entry), false, true));
	}

	void DirMatchingKind_Contains_ReturnsTrue()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry, B_DIRECTORY_NODE) == true);
	}

	void DirMismatchingKind_Contains_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT(dir.Contains(&entry, B_FILE_NODE) == false);
	}

	void FileMatchingKind_Contains_ReturnsTrue()
	{
		// R5 bug: returns false
#if !TEST_R5
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT(dir.Contains(&entry, B_FILE_NODE) == true);
#endif
	}

	void FileMismatchingKind_Contains_ReturnsFalse()
	{
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingFilename));
		CPPUNIT_ASSERT(dir.Contains(&entry, B_SYMLINK_NODE) == false);
	}

	void LinkMatchingKind_Contains_ReturnsTrue()
	{
#if !TEST_R5
		// R5 bug: returns false
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(dirLinkname));
		CPPUNIT_ASSERT(dir.Contains(&entry, B_SYMLINK_NODE) == true);
#endif
	}

	void LinkMismatchingKind_Contains_ReturnsFalse()
	{
		// R5 bug: returns true
#if !TEST_R5
		BDirectory dir;
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(dirLinkname));
		CPPUNIT_ASSERT(dir.Contains(&entry, B_DIRECTORY_NODE) == false);
#endif
	}

	void UninitializedDir_GetStatFor_ReturnsNoInit()
	{
		BDirectory dir;
		struct stat stat1;
		memset(&stat1, 0, sizeof(struct stat));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.GetStatFor(existingDirname, &stat1));
	}

	void BadlyInitializedDir_GetStatFor_ReturnsNoInit()
	{
		BDirectory dir;
		struct stat stat1;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.InitCheck());
		memset(&stat1, 0, sizeof(struct stat));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.GetStatFor(existingDirname, &stat1));
	}

	void InitializedDirAbsolute_GetStatFor_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry;
		struct stat stat1, stat2;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		memset(&stat1, 0, sizeof(struct stat));
		memset(&stat2, 0, sizeof(struct stat));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetStatFor(existingDirname, &stat1));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetStat(&stat2));
		CPPUNIT_ASSERT(stat1 == stat2);
	}

	void InitializedDirRelative_GetStatFor_ReturnsOk()
	{
		BDirectory dir;
		BEntry entry;
		struct stat stat1, stat2;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		memset(&stat1, 0, sizeof(struct stat));
		memset(&stat2, 0, sizeof(struct stat));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetStatFor(existingRelDirname, &stat1));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetStat(&stat2));
		CPPUNIT_ASSERT(stat1 == stat2);
	}

	void NonExistingEntry_GetStatFor_ReturnsNotFound()
	{
		BDirectory dir;
		struct stat mystat;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		memset(&mystat, 0, sizeof(struct stat));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.GetStatFor(nonExistingDirname, &mystat));
	}

	void NullPath_GetStatFor_ReturnsOk()
	{
		BDirectory dir;
		struct stat stat1, stat2;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		memset(&stat1, 0, sizeof(struct stat));
		memset(&stat2, 0, sizeof(struct stat));
		// R5 returns B_OK and the stat structure for the directory
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetStatFor(NULL, &stat1));
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetStat(&stat2));
		CPPUNIT_ASSERT(stat1 == stat2);
	}

	void EmptyPath_GetStatFor_ReturnsNotFound()
	{
		BDirectory dir;
		struct stat stat1;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		// R5 returns B_ENTRY_NOT_FOUND
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.GetStatFor("", &stat1));
	}

	void NullStatAndPath_GetStatFor_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		// R5 returns B_BAD_ADDRESS instead of B_BAD_VALUE
		CPPUNIT_ASSERT(
			equals(dir.GetStatFor(NULL, NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void NullStat_GetStatFor_ReturnsBadValue()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo("/"));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		// R5 returns B_BAD_ADDRESS instead of B_BAD_VALUE
		CPPUNIT_ASSERT(equals(dir.GetStatFor(existingDirname, NULL), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void EmptyDir_GetNextEntry_ReturnsNotFound()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.GetNextEntry(&entry));
		CPPUNIT_ASSERT(dir.CountEntries() == 0);
		rmdir(testDirname1);
	}

	void NonEmptyDir_GetNextEntry_ReturnsEntries()
	{
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		creat((dirPathName + "file1").c_str(), S_IRWXU);
		creat((dirPathName + "file2").c_str(), S_IRWXU);

		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		TestSet testSet;
		testSet.add("file1");
		testSet.add("file2");

		while (dir.GetNextEntry(&entry) == B_OK) {
			BPath path;
			CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
			CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
			CPPUNIT_ASSERT(testSet.test(path.Leaf()) == true);
		}
		CPPUNIT_ASSERT(testSet.testDone() == true);

		rmdir(testDirname1);
	}

	void EmptyDir_GetNextRef_ReturnsNotFound()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.GetNextRef(&ref));
		CPPUNIT_ASSERT(dir.CountEntries() == 0);
		rmdir(testDirname1);
	}

	void NonEmptyDir_GetNextRef_ReturnsEntries()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		creat((dirPathName + "file1").c_str(), S_IRWXU);
		creat((dirPathName + "file2").c_str(), S_IRWXU);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		entry_ref ref;
		TestSet testSet;
		testSet.add("file1");
		testSet.add("file2");

		while (dir.GetNextRef(&ref) == B_OK)
			CPPUNIT_ASSERT(testSet.test(ref.name) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);

		rmdir(testDirname1);
	}

	void EmptyDir_GetNextDirents_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		TestSet testSet;
		testSet.add(".");
		testSet.add("..");
		while (dir.GetNextDirents(ents, bufSize, 1) == 1)
			CPPUNIT_ASSERT(testSet.test(ents->d_name) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);
		rmdir(testDirname1);
	}

	void NonEmptyDir_GetNextDirents_ReturnsEntries()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		creat((dirPathName + "file1").c_str(), S_IRWXU);
		creat((dirPathName + "file2").c_str(), S_IRWXU);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		TestSet testSet;
		testSet.add("file1");
		testSet.add("file2");
		testSet.add(".");
		testSet.add("..");

		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		int count = 0;
		int32 readCount;
		while ((readCount = dir.GetNextDirents(ents, bufSize, 1)) == 1)
			CPPUNIT_ASSERT(testSet.test(ents->d_name) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);
		CPPUNIT_ASSERT(count >= 2);
		rmdir(testDirname1);
	}

	void NonEmptyDir_GetNextEntryAndRewind_ResetsIteration()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		creat((dirPathName + "file1").c_str(), S_IRWXU);
		creat((dirPathName + "file2").c_str(), S_IRWXU);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetNextEntry(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.Rewind());
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetNextEntry(&entry));
		rmdir(testDirname1);
	}

	void NonEmptyDir_CountEntries_ReturnsCorrectCount()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		creat((dirPathName + "file1").c_str(), S_IRWXU);
		creat((dirPathName + "file2").c_str(), S_IRWXU);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(2, dir.CountEntries());
	}

	void NonEmptyDir_NextEntryRefDirentsInterleaved_ReturnsEntries()
	{
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		creat((dirPathName + "file1").c_str(), S_IRWXU);
		creat((dirPathName + "file2").c_str(), S_IRWXU);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());

		BEntry entry;
		entry_ref ref;
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;

		// Just a simple interleave test
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetNextEntry(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetNextRef(&ref));
		dir.Rewind();
		CPPUNIT_ASSERT_EQUAL(1, dir.GetNextDirents(ents, bufSize, 1));
		rmdir(testDirname1);
	}

	void EntryIteration_Uninitialized_ReturnsError()
	{
		BDirectory dir;
		BEntry entry;
		entry_ref ref;
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.GetNextEntry(&entry));
		CPPUNIT_ASSERT(equals(dir.GetNextRef(&ref), B_NO_INIT, B_FILE_ERROR));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.Rewind());
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.GetNextDirents(ents, bufSize, 1));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.CountEntries());
	}

	void EntryIteration_BadlyInitialized_ReturnsError()
	{
		BDirectory dir;
		BEntry entry;
		entry_ref ref;
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;
		// R5: unlike the others GetNextRef() returns B_NO_INIT
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, dir.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.GetNextEntry(&entry));
		CPPUNIT_ASSERT(equals(dir.GetNextRef(&ref), B_NO_INIT, B_FILE_ERROR));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.Rewind());
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.GetNextDirents(ents, bufSize, 1));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, dir.CountEntries());
	}

	void EntryIteration_BadArgs_ReturnsError()
	{
		// R5 crashs, when passing a NULL BEntry or entry_ref
#if !TEST_R5
		BDirectory dir("/");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.GetNextEntry(NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, dir.GetNextRef(NULL));
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		CPPUNIT_ASSERT(
			equals(dir.GetNextDirents(NULL, bufSize, 1),
				   B_BAD_ADDRESS, B_BAD_VALUE));
#endif
	}

	void NonEmptyDirMixedEntries_GetNextEntry_ReturnsAll()
	{
		TestSet testSet;
		const char* iterDir = _CreateMultifileDirStructure(testSet);

		BDirectory dir(iterDir);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());

		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(iterDir));
		testSet.test(".");
		testSet.test("..");
		while (dir.GetNextEntry(&entry) == B_OK) {
			BPath path;
			CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
			CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
			CPPUNIT_ASSERT(testSet.test(path.Leaf()) == true);
		}
		CPPUNIT_ASSERT(testSet.testDone() == true);

		rmdir(iterDir);
	}

	void NonEmptyDirMixedEntries_GetNextRef_ReturnsAll()
	{
		TestSet testSet;
		const char* iterDir = _CreateMultifileDirStructure(testSet);

		BDirectory dir(iterDir);
		entry_ref ref;
		ref.device = -1;
		ref.set_name("");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(iterDir));
		testSet.test(".");
		testSet.test("..");
		while (dir.GetNextRef(&ref) == B_OK)
			CPPUNIT_ASSERT(testSet.test(ref.name) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);

		rmdir(iterDir);
	}

	void NonEmptyDirMixedEntries_GetNextDirents_ReturnsAll()
	{
		TestSet testSet;
		const char* iterDir = _CreateMultifileDirStructure(testSet);

		BDirectory dir(iterDir);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(iterDir));
		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;

		while (dir.GetNextDirents(ents, bufSize, 1) == 1)
			CPPUNIT_ASSERT(testSet.test(ents->d_name) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);

		rmdir(iterDir);
	}

	void NonEmptyDirMixedEntries_CountEntries_ReturnsAll()
	{
		TestSet testSet;
		const char* iterDir = _CreateMultifileDirStructure(testSet);

		BDirectory dir(iterDir);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(iterDir));

		CPPUNIT_ASSERT(dir.CountEntries() == 9);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetNextRef(&ref));
		CPPUNIT_ASSERT(dir.CountEntries() == 9);

		rmdir(iterDir);
	}

	void NonEmptyDirMixedEntries_GetNextEntryRefDirents_ReturnsAll()
	{
		TestSet testSet;
		const char* iterDir = _CreateMultifileDirStructure(testSet);

		BDirectory dir(iterDir);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(iterDir));

		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, entry.InitCheck());

		entry_ref ref;
		ref.device = -1;
		ref.set_name("");

		const size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent* ents = (dirent*)buffer;

		while (dir.GetNextDirents(ents, bufSize, 1) == 1) {
			CPPUNIT_ASSERT(testSet.test(ents->d_name) == true);
			if (dir.GetNextRef(&ref) == B_OK)
				CPPUNIT_ASSERT(testSet.test(ref.name) == true);
			if (dir.GetNextEntry(&entry) == B_OK) {
				BPath path;
				CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
				CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetPath(&path));
				CPPUNIT_ASSERT(testSet.test(path.Leaf()) == true);
			}
		}
		testSet.test(".", false);	// in case they have been skipped
		testSet.test("..", false);	//
		CPPUNIT_ASSERT(testSet.testDone() == true);
		rmdir(iterDir);
	}

	void DirWithLinks_GetNextEntryWithTraverseLinks_ReturnsTarget()
	{
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		symlink(existingFilename, std::string(dirPathName + "link1").c_str());

		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.GetNextEntry(&entry, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BEntry entry2(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry2.InitCheck());
		CPPUNIT_ASSERT(entry == entry2);
		rmdir(testDirname1);
	}

	void RelativePath_CreateDirectory_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());

		BDirectory subdir;
		std::string dirPathName(std::string (testDirname1) + "/");
		std::string entryName("subdir1");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.GetEntry(&entry));
		CPPUNIT_ASSERT(entry == BEntry((dirPathName + entryName).c_str()));
		rmdir(testDirname1);
#endif
	}

	void RelativePath_CreateFile_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());

		BFile file;
		std::string entryName("file1");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void RelativePath_CreateFileWithFailIfExistsFalse_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string (testDirname1) + "/file1").c_str(), S_IRWXU);
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());

		BFile file;
		std::string entryName("file1");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void RelativePath_CreateSymLink_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());

		BSymLink link;
		std::string entryName("link1");
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateSymLink(entryName.c_str(), existingFilename, &link));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void AbsolutePath_CreateDirectory_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));
		std::string dirPathName(std::string (testDirname1) + "/");

		BDirectory subdir;
		std::string entryName = dirPathName + "subdir_abs";
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void AbsolutePath_CreateFile_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));

		BFile file;
		std::string entryName = std::string(testDirname1) + "/file_abs";
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void AbsolutePath_CreateFileWithFailIfExistsFalse_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file_abs").c_str(), S_IRWXU);
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));

		BFile file;
		std::string entryName = std::string(testDirname1) + "/file_abs";
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void AbsolutePath_CreateSymLink_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.SetTo(existingDirname));

		BSymLink link;
		std::string entryName = std::string(testDirname1) + "/link_abs";
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateSymLink(entryName.c_str(), existingFilename, &link));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void ExistingEntries_CreateDirectory_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		BDirectory dir(testDirname1);
		std::string entryName("subdir1");
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, subdir.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void ExistingEntries_CreateFile_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		BDirectory dir(testDirname1);
		std::string entryName("file1");
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void ExistingEntries_CreateFileWithFailIfExistsFalse_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		BDirectory dir(testDirname1);
		std::string entryName("file1");
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, false));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void ExistingEntries_CreateSymLink_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		BDirectory dir(testDirname1);
		std::string entryName("link1");
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateSymLink(entryName.c_str(), "dummy", &link));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndNonExistingEntry_CreateDirectory_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "subdir1";
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.InitCheck());
		BEntry entry;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.GetEntry(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		CPPUNIT_ASSERT(entry == BEntry(entryName.c_str()));
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndExistingEntry_CreateDirectory_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "subdir1";
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, subdir.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndNonExistingEntry_CreateFile_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetTo(entryName.c_str(), B_READ_ONLY));
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndExistingEntry_CreateFile_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndExistingEntry_CreateFileWithFailIfExistsFalse_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndNonExistingEntry_CreateSymLink_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "link1";
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateSymLink(entryName.c_str(), existingFilename, &link));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(entryName.c_str()));
		rmdir(testDirname1);
#endif
	}

	void UninitializedDirAndExistingEntry_CreateSymLink_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "link1";
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateSymLink(entryName.c_str(), existingFilename, &link));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndNonExistingEntry_CreateDirectory_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "subdir1";
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo((dirPathName + entryName).c_str()));
		rmdir(entryName.c_str());
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndExistingEntry_CreateDirectory_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "subdir1";
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateDirectory(entryName.c_str(), &subdir));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, subdir.InitCheck());
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndNonExistingEntry_CreateFile_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.SetTo((dirPathName + entryName).c_str(), B_READ_ONLY) == B_OK);
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndExistingEntry_CreateFile_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateFile(entryName.c_str(), &file, true));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndExistingEntry_CreateFileWithFailIfExistsFalse_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), &file, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndNonExistingEntry_CreateSymLink_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "link1";
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateSymLink(entryName.c_str(), existingFilename, &link));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo((dirPathName + entryName).c_str()));
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void RelativePathUninitializedDirAndExistingEntry_CreateSymLink_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		chdir(testDirname1);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = "link1";
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateSymLink(entryName.c_str(), existingFilename, &link));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		chdir("/");
		rmdir(testDirname1);
#endif
	}

	void NullEntryName_CreateDirectory_ReturnsBadValue()
	{
#ifndef TEST_R5
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BDirectory subdir;
		CPPUNIT_ASSERT(
			equals(dir.CreateDirectory(NULL, &subdir),
				   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, subdir.InitCheck());
#endif
	}

	void NullEntryName_CreateFile_ReturnsError()
	{
#ifndef TEST_R5
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		// R5: unlike CreateDirectory/SymLink() CreateFile() returns
		//	   B_ENTRY_NOT_FOUND
		BFile file;
		CPPUNIT_ASSERT(equals(dir.CreateFile(NULL, &file), B_ENTRY_NOT_FOUND,
							  B_BAD_VALUE));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
#endif
	}

	void NullArgs_CreateSymLink_ReturnsBadValue()
	{
#ifndef TEST_R5
		BDirectory dir(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BSymLink link;
		CPPUNIT_ASSERT(
			equals(dir.CreateSymLink(NULL, existingFilename, &link),
				   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
		CPPUNIT_ASSERT(
			equals(dir.CreateSymLink("link1", NULL, &link),
				   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, link.InitCheck());
#endif
	}

	void NullOutParamAndNonExistingEntry_CreateDirectory_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "subdir1";
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateDirectory(entryName.c_str(), NULL));
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		rmdir(testDirname1);
#endif
	}

	void NullOutParamAndExistingEntry_CreateDirectory_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		mkdir((std::string(testDirname1) + "/subdir1").c_str(), S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "subdir1";
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateDirectory(entryName.c_str(), NULL));
		rmdir(testDirname1);
#endif
	}

	void NullOutParamAndNonExistingEntry_CreateFile_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), NULL, true));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetTo(entryName.c_str(), B_READ_ONLY));
		rmdir(testDirname1);
#endif
	}

	void NullOutParamAndExistingEntry_CreateFile_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateFile(entryName.c_str(), NULL, true));
		rmdir(testDirname1);
#endif
	}

	void NullOutParamAndExistingEntry_CreateFileWithFailIfExistsFalse_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		creat((std::string(testDirname1) + "/file1").c_str(), S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "file1";
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateFile(entryName.c_str(), NULL, false));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetTo(entryName.c_str(), B_READ_ONLY));
		rmdir(testDirname1);
#endif
	}

	void NullOutParamAndNonExistingEntry_CreateSymLink_ReturnsOk()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "link1";
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.CreateSymLink(entryName.c_str(), existingFilename, NULL));
		CPPUNIT_ASSERT_EQUAL(B_OK, link.SetTo(entryName.c_str()));
		rmdir(testDirname1);
#endif
	}

	void NullOutParamAndExistingEntry_CreateSymLink_ReturnsFileExists()
	{
#ifndef TEST_R5
		mkdir(testDirname1, S_IRWXU);
		symlink("dummy", (std::string(testDirname1) + "/link1").c_str());
		std::string dirPathName(std::string (testDirname1) + "/");
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		std::string entryName = dirPathName + "link1";
		BSymLink link;
		CPPUNIT_ASSERT_EQUAL(B_FILE_EXISTS, dir.CreateSymLink(entryName.c_str(), existingFilename, NULL));
		rmdir(testDirname1);
#endif
	}

	void UninitializedDir_CopyConstructor_ReturnsNoInit()
	{
		BDirectory dir;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, dir.InitCheck());
		BDirectory dir2(dir);
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(dir2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void ExistingDir_CopyConstructor_ReturnsOk()
	{
		BDirectory dir(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BDirectory dir2(dir);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir2.InitCheck());
	}

	void UninitializedToUninitialized_Assignment_ReturnsNoInit()
	{
		BDirectory dir;
		BDirectory dir2;
		dir2 = dir;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(dir2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void UninitializedToExisting_Assignment_ReturnsNoInit()
	{
		BDirectory dir;
		BDirectory dir2(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir2.InitCheck());
		dir2 = dir;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(dir2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void Assignment_Operator_AssignsCorrectly()
	{
		BDirectory dir(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, dir.InitCheck());
		BDirectory dir2;
		dir2 = dir;
		CPPUNIT_ASSERT_EQUAL(B_OK, dir2.InitCheck());
	}

	void AbsolutePathAndNestedPath_CreateDirectoryCFunc_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		std::string entryName(dirPathName + "subdir1/subdir1.1");
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(entryName.c_str(), 0x1ff));
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		rmdir(testDirname1);
	}

	void AbsolutePathAndSingleLevel_CreateDirectoryCFunc_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		std::string dirPathName(std::string (testDirname1) + "/");
		std::string entryName = dirPathName + "subdir2";
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(entryName.c_str(), 0x1ff));
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		rmdir(testDirname1);
	}

	void AbsolutePathAndExistingDir_CreateDirectoryCFunc_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		std::string entryName(testDirname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(entryName.c_str(), 0x1ff));
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		rmdir(testDirname1);
	}

	void RelativePathAndNestedPath_CreateDirectoryCFunc_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		chdir(testDirname1);
		std::string entryName = "subdir1/subdir1.1";
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(entryName.c_str(), 0x1ff));
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		chdir("/");
		rmdir(testDirname1);
	}

	void RelativePathAndSingleLevel_CreateDirectoryCFunc_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		chdir(testDirname1);
		std::string entryName = "subdir2";
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(entryName.c_str(), 0x1ff));
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		chdir("/");
		rmdir(testDirname1);
	}

	void RelativePathAndExistingDir_CreateDirectoryCFunc_ReturnsOk()
	{
		mkdir(testDirname1, S_IRWXU);
		chdir(testDirname1);
		std::string entryName = ".";
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(entryName.c_str(), 0x1ff));
		BDirectory subdir;
		CPPUNIT_ASSERT_EQUAL(B_OK, subdir.SetTo(entryName.c_str()));
		chdir("/");
		rmdir(testDirname1);
	}

	void ExistingFileOrLink_CreateDirectoryCFunc_ReturnsExpected()
	{
		CPPUNIT_ASSERT(
			equals(create_directory(existingFilename, 0x1ff), B_BAD_VALUE, B_NOT_A_DIRECTORY));
		CPPUNIT_ASSERT(equals(create_directory(fileLinkname, 0x1ff), B_BAD_VALUE, B_NOT_A_DIRECTORY));
		CPPUNIT_ASSERT_EQUAL(B_OK, create_directory(dirLinkname, 0x1ff));
	}

	void NullPath_CreateDirectoryCFunc_ReturnsBadValue()
	{
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, create_directory(NULL, 0x1ff));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DirectoryTest, getTestSuiteName());
