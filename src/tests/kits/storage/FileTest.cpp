/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "Errors.h"
#include "NodeTest.h"
#include "StorageDefs.h"
#include "app/RegistrarDefs.h"
#include "cppunit/TestAssert.h"

class FileTest : public NodeTest {
	CPPUNIT_TEST_SUB_SUITE(FileTest, NodeTest);
	CPPUNIT_TEST(DefaultConstructor_ReturnsNoInit);
	CPPUNIT_TEST(ExistingFile_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(NonExistingFile_PathConstructor_ReturnsExpected);
	CPPUNIT_TEST(Null_PathConstructor_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_EntryRefConstructor_ReturnsExpected);
	CPPUNIT_TEST(NonExistingFile_EntryRefConstructor_ReturnsExpected);
	CPPUNIT_TEST(Null_EntryRefConstructor_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_EntryConstructor_ReturnsExpected);
	CPPUNIT_TEST(NonExistingFile_EntryConstructor_ReturnsExpected);
	CPPUNIT_TEST(Null_EntryConstructor_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_DirAndPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(NonExistingFile_DirAndPathConstructor_ReturnsExpected);
	CPPUNIT_TEST(Null_DirAndPathConstructor_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_SetToPath_ReturnsOk);
	CPPUNIT_TEST(NonExistingFile_SetToPath_ReturnsNotFound);
	CPPUNIT_TEST(Null_SetToPath_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_SetToEntryRef_ReturnsOk);
	CPPUNIT_TEST(NonExistingFile_SetToEntryRef_ReturnsNotFound);
	CPPUNIT_TEST(Null_SetToEntryRef_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(NonExistingFile_SetToEntry_ReturnsNotFound);
	CPPUNIT_TEST(Null_SetToEntry_ReturnsBadValue);

	CPPUNIT_TEST(ExistingFile_SetToDirAndPath_ReturnsOk);
	CPPUNIT_TEST(NonExistingFile_SetToDirAndPath_ReturnsNotFound);
	CPPUNIT_TEST(Null_SetToDirAndPath_ReturnsBadValue);

	CPPUNIT_TEST(Uninitialized_IsReadableWritable_ReturnsFalse);
	CPPUNIT_TEST(ReadOnly_IsReadableWritable_ReturnsReadableOnly);
	CPPUNIT_TEST(WriteOnly_IsReadableWritable_ReturnsWritableOnly);
	CPPUNIT_TEST(ReadWrite_IsReadableWritable_ReturnsBoth);
	CPPUNIT_TEST(NonExisting_IsReadableWritable_ReturnsFalse);
	CPPUNIT_TEST(Uninitialized_ReadWrite_ReturnsError);
	CPPUNIT_TEST(ReadOnlyAndWriteOnly_ReadWrite_ReturnsError);
	CPPUNIT_TEST(EmptyFile_Read_ReturnsZero);
	CPPUNIT_TEST(ExistingFile_ReadPastEmptyFile_ReturnsZero);
	CPPUNIT_TEST(CreateWriteOnlyEraseFlags_ReadWrite_ReturnsOk);
	CPPUNIT_TEST(CreateWriteOnlyEraseFlags_ReadAtWriteAt_ReturnsOk);
	CPPUNIT_TEST(CreateWriteOnlyEraseFlags_WritePastEndOfFile_ReturnsOk);
	CPPUNIT_TEST(CreateWriteOnlyEraseOpenAtEndFlags_Write_ReturnsOk);
	CPPUNIT_TEST(CreateWriteOnlyEraseOpenAtEndFlags_WriteAt_ReturnsOk);
	CPPUNIT_TEST(OpenAtEndFlag_ReadAt_ReturnsOk);
	CPPUNIT_TEST(OpenAtEndFlag_SeekAndRead_ReturnsOk);

	CPPUNIT_TEST(Uninitialized_Position_ReturnsError);
	CPPUNIT_TEST(WriteAndSeek_Position_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_Size_ReturnsError);
	CPPUNIT_TEST(ReadOnly_Size_SetSizeFails);
	CPPUNIT_TEST(WriteOnly_Size_ReturnsOk);
	CPPUNIT_TEST(Enlarge_Size_ReturnsOk);
	CPPUNIT_TEST(EraseFile_Size_ReturnsOk);
	CPPUNIT_TEST(EraseFileUsingSetSize_Size_ReturnsOk);

	CPPUNIT_TEST(Uninitialized_CopyConstructor_ReturnsExpected);
	CPPUNIT_TEST(ReadOnly_CopyConstructor_ReturnsExpected);
	CPPUNIT_TEST(WriteOnly_CopyConstructor_ReturnsExpected);
	CPPUNIT_TEST(ReadWrite_CopyConstructor_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_Assignment_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_AssignmentToInitialized_ReturnsExpected);
	CPPUNIT_TEST(ReadOnly_Assignment_ReturnsExpected);
	CPPUNIT_TEST(WriteOnly_Assignment_ReturnsExpected);
	CPPUNIT_TEST(ReadWrite_Assignment_ReturnsExpected);
	CPPUNIT_TEST_SUITE_END();

	static const int32 kAccessModes[3];

protected:
	virtual void CreateRONodes(TestNodes& testEntries)
	{
		testEntries.clear();
		const char *filename;
		filename = existingFilename;
		testEntries.add(new BFile(filename, B_READ_ONLY), filename);
	}

	virtual void CreateRWNodes(TestNodes& testEntries)
	{
		testEntries.clear();
		const char *filename;
		filename = existingFilename;
		testEntries.add(new BFile(filename, B_READ_WRITE), filename);
	}

	virtual void CreateUninitializedNodes(TestNodes& testEntries)
	{
		testEntries.clear();
		testEntries.add(new BFile, "");
	}

	status_t _ConstructReturn(int accessMode, int flagsNum, bool existing)
	{
		// 1 - B_CREATE_FILE
		// 2 - B_FAIL_IF_EXISTS
		// 4 - B_ERASE_FILE
		CPPUNIT_ASSERT_MESSAGE("Invalid flag combination", flagsNum != 6);
		if (existing) {
			switch(flagsNum) {
				case 4: // B_CREATE_FILE
				case 5: // B_CREATE_FILE + B_ERASE_FILE
					return accessMode == B_READ_ONLY ? B_NOT_ALLOWED : B_OK;
				case 3: // B_CREATE_FILE + B_FAIL_IF_EXISTS
				case 7: // B_CREATE_FILE + B_FAIL_IF_EXISTS + B_ERASE_FILE
					return B_FILE_EXISTS;
				default:
					return B_OK;
			}
		}
		else {
			switch(flagsNum) {
				case 0:
				case 2: // B_FAIL_IF_EXISTS
				case 4: // B_ERASE_FILE
					return B_ENTRY_NOT_FOUND;
				default:
					return B_OK;
			}
		}
	}

	int32 _ConstructFlags(int accessMode, int flagsNum)
	{
		int32 flags = accessMode;
		flags |= (flagsNum & 1) ? B_CREATE_FILE : 0;
		flags |= (flagsNum & 2) ? B_FAIL_IF_EXISTS : 0;
		flags |= (flagsNum & 4) ? B_ERASE_FILE : 0;
		return flags;
	}

public:
	void setUp()
	{
		NodeTest::setUp();
	}

	void tearDown()
	{
		NodeTest::tearDown();
	}

	void DefaultConstructor_ReturnsNoInit()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
	}

	void ExistingFile_PathConstructor_ReturnsExpected()
	{
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(existingFilename, _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true), file.InitCheck());
			}
		}
	}

	void NonExistingFile_PathConstructor_ReturnsExpected()
	{
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(nonExistingFilename, _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false), file.InitCheck());
			}
		}
	}

	void Null_PathConstructor_ReturnsBadValue()
	{
		BFile file((const char*)NULL, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.InitCheck());
	}

	void ExistingFile_EntryRefConstructor_ReturnsExpected()
	{
		BEntry entry(existingFilename);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(&ref, _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true), file.InitCheck());
			}
		}
	}

	void NonExistingFile_EntryRefConstructor_ReturnsExpected()
	{
		BEntry entry(nonExistingFilename);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(&ref, _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false), file.InitCheck());
			}
		}
	}

	void Null_EntryRefConstructor_ReturnsBadValue()
	{
		BFile file((entry_ref*)NULL, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.InitCheck());
	}

	void ExistingFile_EntryConstructor_ReturnsExpected()
	{
		BEntry entry(existingFilename);
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(&entry, _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true), file.InitCheck());
			}
		}
	}

	void NonExistingFile_EntryConstructor_ReturnsExpected()
	{
		BEntry entry(nonExistingFilename);
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(&entry, _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false), file.InitCheck());
			}
		}
	}

	void Null_EntryConstructor_ReturnsBadValue()
	{
		BFile file((BEntry*)NULL, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.InitCheck());
	}

	void ExistingFile_DirAndPathConstructor_ReturnsExpected()
	{
		BPath path(existingFilename);
		BPath dirPath;
		path.GetParent(&dirPath);
		BDirectory dir(dirPath.Path());
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(&dir, path.Leaf(), _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true), file.InitCheck());
			}
		}
	}

	void NonExistingFile_DirAndPathConstructor_ReturnsExpected()
	{
		BPath path(nonExistingFilename);
		BPath dirPath;
		path.GetParent(&dirPath);
		BDirectory dir(dirPath.Path());
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file(&dir, path.Leaf(), _ConstructFlags(kAccessModes[access], flags));
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false), file.InitCheck());
			}
		}
	}

	void Null_DirAndPathConstructor_ReturnsBadValue()
	{
		BFile file((BDirectory*)NULL, "file", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.InitCheck());
	}

	void ExistingFile_SetToPath_ReturnsOk()
	{
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true),
					file.SetTo(existingFilename, _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void NonExistingFile_SetToPath_ReturnsNotFound()
	{
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false),
					file.SetTo(nonExistingFilename, _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void Null_SetToPath_ReturnsBadValue()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.SetTo((const char*)NULL, B_READ_ONLY));
	}

	void ExistingFile_SetToEntryRef_ReturnsOk()
	{
		BEntry entry(existingFilename);
		entry_ref ref;
		entry.GetRef(&ref);
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true),
					file.SetTo(&ref, _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void NonExistingFile_SetToEntryRef_ReturnsNotFound()
	{
		BEntry entry(nonExistingFilename);
		entry_ref ref;
		entry.GetRef(&ref);
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false),
					file.SetTo(&ref, _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void Null_SetToEntryRef_ReturnsBadValue()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.SetTo((entry_ref*)NULL, B_READ_ONLY));
	}

	void ExistingFile_SetToEntry_ReturnsOk()
	{
		BEntry entry(existingFilename);
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true),
					file.SetTo(&entry, _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void NonExistingFile_SetToEntry_ReturnsNotFound()
	{
		BEntry entry(nonExistingFilename);
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false),
					file.SetTo(&entry, _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void Null_SetToEntry_ReturnsBadValue()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.SetTo((BEntry*)NULL, B_READ_ONLY));
	}

	void ExistingFile_SetToDirAndPath_ReturnsOk()
	{
		BPath path(existingFilename);
		BPath dirPath;
		path.GetParent(&dirPath);
		BDirectory dir(dirPath.Path());
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, true),
					file.SetTo(&dir, path.Leaf(), _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void NonExistingFile_SetToDirAndPath_ReturnsNotFound()
	{
		BPath path(nonExistingFilename);
		BPath dirPath;
		path.GetParent(&dirPath);
		BDirectory dir(dirPath.Path());
		for (int access = 0; access < 3; access++) {
			for (int flags = 0; flags < 8; flags++) {
				if(flags == 6) continue;
				BFile file;
				CPPUNIT_ASSERT_EQUAL(_ConstructReturn(kAccessModes[access], flags, false),
					file.SetTo(&dir, path.Leaf(), _ConstructFlags(kAccessModes[access], flags)));
			}
		}
	}

	void Null_SetToDirAndPath_ReturnsBadValue()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, file.SetTo((BDirectory*)NULL, "file", B_READ_ONLY));
	}

	void Uninitialized_IsReadableWritable_ReturnsFalse()
	{
		BFile file;
		CPPUNIT_ASSERT(file.IsReadable() == false);
		CPPUNIT_ASSERT(file.IsWritable() == false);
	}

	void ReadOnly_IsReadableWritable_ReturnsReadableOnly()
	{
		BFile file(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT(file.IsReadable() == true);
		CPPUNIT_ASSERT(file.IsWritable() == false);
	}

	void WriteOnly_IsReadableWritable_ReturnsWritableOnly()
	{
		BFile file(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT(file.IsReadable() == false);
		CPPUNIT_ASSERT(file.IsWritable() == true);
	}

	void ReadWrite_IsReadableWritable_ReturnsBoth()
	{
		BFile file(existingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT(file.IsReadable() == true);
		CPPUNIT_ASSERT(file.IsWritable() == true);
	}

	void NonExisting_IsReadableWritable_ReturnsFalse()
	{
		BFile file(nonExistingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT(file.IsReadable() == false);
		CPPUNIT_ASSERT(file.IsWritable() == false);
	}

	void Uninitialized_ReadWrite_ReturnsError()
	{
		BFile file;
		char buffer[10];
		CPPUNIT_ASSERT(file.Read(buffer, sizeof(buffer)) < 0);
		CPPUNIT_ASSERT(file.ReadAt(0, buffer, sizeof(buffer)) < 0);
		CPPUNIT_ASSERT(file.Write(buffer, sizeof(buffer)) < 0);
		CPPUNIT_ASSERT(file.WriteAt(0, buffer, sizeof(buffer)) < 0);
	}

	void ReadOnlyAndWriteOnly_ReadWrite_ReturnsError()
	{
		BFile file;
		char buffer[10];
		file.SetTo(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Read(buffer, sizeof(buffer)) < 0);
		CPPUNIT_ASSERT(file.ReadAt(0, buffer, sizeof(buffer)) < 0);
		file.SetTo(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Write(buffer, sizeof(buffer)) < 0);
		CPPUNIT_ASSERT(file.WriteAt(0, buffer, sizeof(buffer)) < 0);
	}

	void EmptyFile_Read_ReturnsZero()
	{
		BFile file;
		char buffer[10];
		file.SetTo(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Read(buffer, sizeof(buffer)) == 0);
		CPPUNIT_ASSERT(file.ReadAt(0, buffer, sizeof(buffer)) == 0);
	}

	void ExistingFile_ReadPastEmptyFile_ReturnsZero()
	{
		BFile file;
		char buffer[10];
		file.SetTo(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Seek(10, SEEK_SET) == 10);
		CPPUNIT_ASSERT(file.Read(buffer, sizeof(buffer)) == 0);
		CPPUNIT_ASSERT(file.ReadAt(10, buffer, sizeof(buffer)) == 0);
	}

	void CreateWriteOnlyEraseFlags_ReadWrite_ReturnsOk()
	{
		BFile file;
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char writeBuffer[256];
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		CPPUNIT_ASSERT(file.Write(writeBuffer, 128) == 128);
		CPPUNIT_ASSERT(file.Position() == 128);
		CPPUNIT_ASSERT(file.Write(writeBuffer + 128, 128) == 128);
		CPPUNIT_ASSERT(file.Position() == 256);
		file.Unset();
		file.SetTo(testFilename1, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char readBuffer[256];
		CPPUNIT_ASSERT(file.Read(readBuffer, 42) == 42);
		CPPUNIT_ASSERT(file.Position() == 42);
		CPPUNIT_ASSERT(file.Read(readBuffer + 42, 400) == 214);
		CPPUNIT_ASSERT(file.Position() == 256);
		for (int32 i = 0; i < 256; i++)
			CPPUNIT_ASSERT(readBuffer[i] == (char)i);
		file.Unset();
		unlink(testFilename1);
	}

	void CreateWriteOnlyEraseFlags_ReadAtWriteAt_ReturnsOk()
	{
		BFile file;
		char writeBuffer[256];
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.WriteAt(80, writeBuffer + 80, 50) == 50);
		CPPUNIT_ASSERT(file.Position() == 0);
		CPPUNIT_ASSERT(file.WriteAt(0, writeBuffer, 80) == 80);
		CPPUNIT_ASSERT(file.Position() == 0);
		CPPUNIT_ASSERT(file.WriteAt(130, writeBuffer + 130, 126) == 126);
		CPPUNIT_ASSERT(file.Position() == 0);
		file.Unset();
		file.SetTo(testFilename1, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char readBuffer[256];
		for (int32 i = 0; i < 256; i++)
			readBuffer[i] = 0;
		CPPUNIT_ASSERT(file.ReadAt(42, readBuffer + 42, 84) == 84);
		CPPUNIT_ASSERT(file.Position() == 0);
		CPPUNIT_ASSERT(file.ReadAt(0, readBuffer, 42) == 42);
		CPPUNIT_ASSERT(file.Position() == 0);
		CPPUNIT_ASSERT(file.ReadAt(126, readBuffer + 126, 130) == 130);
		CPPUNIT_ASSERT(file.Position() == 0);
		for (int32 i = 0; i < 256; i++)
			CPPUNIT_ASSERT(readBuffer[i] == (char)i);
		file.Unset();
		unlink(testFilename1);
	}

	void CreateWriteOnlyEraseFlags_WritePastEndOfFile_ReturnsOk()
	{
		BFile file;
		char writeBuffer[256];
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Seek(128, SEEK_SET) == 128);
		CPPUNIT_ASSERT(file.Write(writeBuffer, 128) == 128);
		CPPUNIT_ASSERT(file.Position() == 256);
		file.Unset();
		unlink(testFilename1);
	}

	void CreateWriteOnlyEraseOpenAtEndFlags_Write_ReturnsOk()
	{
		BFile file;
		char writeBuffer[256];
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		CPPUNIT_ASSERT(file.Write(writeBuffer, 256) == 256);
		file.Unset();
		file.SetTo(testFilename1, B_WRITE_ONLY | B_OPEN_AT_END);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)7;
		CPPUNIT_ASSERT(file.Write(writeBuffer, 50) == 50);
		file.Seek(0, SEEK_SET);	// might fail -- don't check the return value
		CPPUNIT_ASSERT(file.Write(writeBuffer, 40) == 40);
		file.Unset();
		file.SetTo(testFilename1, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char readBuffer[256];
		CPPUNIT_ASSERT(file.ReadAt(256, readBuffer, 90) == 90);
		for (int32 i = 0; i < 90; i++)
			CPPUNIT_ASSERT(readBuffer[i] == 7);
		file.Unset();
		unlink(testFilename1);
	}

	void CreateWriteOnlyEraseOpenAtEndFlags_WriteAt_ReturnsOk()
	{
		BFile file;
		char writeBuffer[256];
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		CPPUNIT_ASSERT(file.Write(writeBuffer, 256) == 256);
		file.Unset();
		file.SetTo(testFilename1, B_WRITE_ONLY | B_OPEN_AT_END);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)42;
		CPPUNIT_ASSERT(file.WriteAt(0, writeBuffer, 30) == 30);
		file.Unset();
		file.SetTo(testFilename1, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char readBuffer[256];
		CPPUNIT_ASSERT(file.ReadAt(256, readBuffer, 30) == 30);
		for (int32 i = 0; i < 30; i++)
			CPPUNIT_ASSERT(readBuffer[i] == 42);
		file.Unset();
		unlink(testFilename1);
	}

	void OpenAtEndFlag_ReadAt_ReturnsOk()
	{
		BFile file;
		char writeBuffer[256];
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		CPPUNIT_ASSERT(file.Write(writeBuffer, 256) == 256);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)7;
		CPPUNIT_ASSERT(file.WriteAt(256, writeBuffer, 90) == 90);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)42;
		CPPUNIT_ASSERT(file.WriteAt(346, writeBuffer, 30) == 30);
		file.Unset();

		file.SetTo(testFilename1, B_READ_ONLY | B_OPEN_AT_END);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char readBuffer[256];
		for (int32 i = 0; i < 256; i++)
			readBuffer[i] = 0;
		CPPUNIT_ASSERT(file.ReadAt(256, readBuffer, 90) == 90);
		for (int32 i = 0; i < 90; i++)
			CPPUNIT_ASSERT(readBuffer[i] == 7);
		CPPUNIT_ASSERT(file.ReadAt(346, readBuffer, 30) == 30);
		for (int32 i = 0; i < 30; i++)
			CPPUNIT_ASSERT(readBuffer[i] == 42);
		file.Unset();
		unlink(testFilename1);
	}

	void OpenAtEndFlag_SeekAndRead_ReturnsOk()
	{
		BFile file;
		char writeBuffer[256];
		// 256 bytes pattern + 90 bytes of 7s + 30 bytes of 42s
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)i;
		CPPUNIT_ASSERT(file.Write(writeBuffer, 256) == 256);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)7;
		CPPUNIT_ASSERT(file.WriteAt(256, writeBuffer, 90) == 90);
		for (int32 i = 0; i < 256; i++)
			writeBuffer[i] = (char)42;
		CPPUNIT_ASSERT(file.WriteAt(346, writeBuffer, 30) == 30);
		file.Unset();

		file.SetTo(testFilename1, B_READ_ONLY | B_OPEN_AT_END);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		char readBuffer[256];
		for (int32 i = 0; i < 256; i++)
			readBuffer[i] = 0;
		file.Seek(256, SEEK_SET);	// might fail -- don't check the return value
		CPPUNIT_ASSERT(file.Read(readBuffer, 90) == 90);
		for (int32 i = 0; i < 90; i++)
			CPPUNIT_ASSERT(readBuffer[i] == 7);
		CPPUNIT_ASSERT(file.ReadAt(346, readBuffer, 30) == 30);
		for (int32 i = 0; i < 30; i++)
			CPPUNIT_ASSERT(readBuffer[i] == 42);
		file.Unset();
		unlink(testFilename1);
	}

	void Uninitialized_Position_ReturnsError()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, file.Position());
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, file.Seek(10, SEEK_SET));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, file.Seek(10, SEEK_END));
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, file.Seek(10, SEEK_CUR));
	}

	void WriteAndSeek_Position_ReturnsOk()
	{
		BFile file;
		file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Position() == 0);
		char writeBuffer[256];
		CPPUNIT_ASSERT(file.Write(writeBuffer, 256) == 256);
		CPPUNIT_ASSERT(file.Position() == 256);
		CPPUNIT_ASSERT(file.Seek(10, SEEK_SET) == 10);
		CPPUNIT_ASSERT(file.Position() == 10);
		CPPUNIT_ASSERT(file.Seek(-20, SEEK_END) == 236);
		CPPUNIT_ASSERT(file.Position() == 236);
		CPPUNIT_ASSERT(file.Seek(-70, SEEK_CUR) == 166);
		CPPUNIT_ASSERT(file.Position() == 166);
		file.Unset();
		// re-open the file at the end and seek a bit around once more
		// The BeBook is a bit unspecific about the B_OPEN_AT_END flag:
		// It has probably the same meaning as the POSIX flag O_APPEND, which
		// means, that all write()s append their data at the end. The behavior
		// of Seek() and Position() is a bit unclear for this case.
	/*
		file.SetTo(testFilename1, B_READ_ONLY | B_OPEN_AT_END);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		CPPUNIT_ASSERT(file.Position() == 256);
		CPPUNIT_ASSERT(file.Seek(10, SEEK_SET) == 10);
		CPPUNIT_ASSERT(file.Position() == 10);			// fails with R5
		CPPUNIT_ASSERT(file.Seek(-20, SEEK_END) == 236);
		CPPUNIT_ASSERT(file.Position() == 236);			// fails with R5
		CPPUNIT_ASSERT(file.Seek(-70, SEEK_CUR) == 166);	// fails with R5
		CPPUNIT_ASSERT(file.Position() == 166);			// fails with R5
	*/
		unlink(testFilename1);
	}

	void Uninitialized_Size_ReturnsError()
	{
		BFile file;
		off_t size;
		CPPUNIT_ASSERT(file.GetSize(&size) != B_OK);
		CPPUNIT_ASSERT(file.SetSize(100) != B_OK);
	}

	void ReadOnly_Size_SetSizeFails()
	{
		BFile file(testFilename1, B_READ_ONLY | B_CREATE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		off_t size;
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 0);
		CPPUNIT_ASSERT_EQUAL(file.SetSize(100), B_BAD_VALUE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT_EQUAL(size, 0);
		file.Unset();
		unlink(testFilename1);
	}

	void WriteOnly_Size_ReturnsOk()
	{
		int fd = creat(testFilename1, S_IRWXU);
		close(fd);
		BFile file;
		file.SetTo(testFilename1, B_WRITE_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		off_t size;
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT_EQUAL(size, 0);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetSize(73));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 73);
		file.Unset();
		unlink(testFilename1);
	}

	void Enlarge_Size_ReturnsOk()
	{
		int fd = creat(testFilename1, S_IRWXU);
		close(fd);
		BFile file;
		file.SetTo(testFilename1, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		off_t size;
		file.SetSize(73);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 73);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetSize(147));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 147);
		file.Unset();
		unlink(testFilename1);
	}

	void EraseFile_Size_ReturnsOk()
	{
		int fd = creat(testFilename1, S_IRWXU);
		write(fd, testFilename1, 4);
		close(fd);
		BFile file;
		file.SetTo(testFilename1, B_WRITE_ONLY | B_ERASE_FILE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		off_t size;
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 0);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetSize(93));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 93);
		file.Unset();
		unlink(testFilename1);
	}

	void EraseFileUsingSetSize_Size_ReturnsOk()
	{
		int fd = creat(testFilename1, S_IRWXU);
		write(fd, testFilename1, 4);
		close(fd);
		BFile file;
		file.SetTo(testFilename1, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		file.SetSize(93);
		off_t size;
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 93);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.SetSize(0));
		CPPUNIT_ASSERT_EQUAL(B_OK, file.GetSize(&size));
		CPPUNIT_ASSERT(size == 0);
		file.Unset();
		unlink(testFilename1);
	}

	void Uninitialized_CopyConstructor_ReturnsExpected()
	{
		BFile file;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, file.InitCheck());
		BFile file2(file);
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(file2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void ReadOnly_CopyConstructor_ReturnsExpected()
	{
		BFile file(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BFile file2(file);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT(file2.IsReadable() == true);
		CPPUNIT_ASSERT(file2.IsWritable() == false);
	}

	void WriteOnly_CopyConstructor_ReturnsExpected()
	{
		BFile file(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BFile file2(file);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT(file2.IsReadable() == false);
		CPPUNIT_ASSERT(file2.IsWritable() == true);
	}

	void ReadWrite_CopyConstructor_ReturnsExpected()
	{
		BFile file(existingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BFile file2(file);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT(file2.IsReadable() == true);
		CPPUNIT_ASSERT(file2.IsWritable() == true);
	}

	void Uninitialized_Assignment_ReturnsExpected()
	{
		BFile file;
		BFile file2;
		file2 = file;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(file2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void Uninitialized_AssignmentToInitialized_ReturnsExpected()
	{
		BFile file;
		BFile file2(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		file2 = file;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT(equals(file2.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void ReadOnly_Assignment_ReturnsExpected()
	{
		BFile file(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BFile file2;
		file2 = file;
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT(file2.IsReadable() == true);
		CPPUNIT_ASSERT(file2.IsWritable() == false);
	}

	void WriteOnly_Assignment_ReturnsExpected()
	{
		BFile file(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BFile file2;
		file2 = file;
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT(file2.IsReadable() == false);
		CPPUNIT_ASSERT(file2.IsWritable() == true);
	}

	void ReadWrite_Assignment_ReturnsExpected()
	{
		BFile file(existingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());
		BFile file2;
		file2 = file;
		CPPUNIT_ASSERT_EQUAL(B_OK, file2.InitCheck());
		CPPUNIT_ASSERT(file2.IsReadable() == true);
		CPPUNIT_ASSERT(file2.IsWritable() == true);
	}
};


const int32 FileTest::kAccessModes[3] = { B_READ_ONLY, B_WRITE_ONLY, B_READ_WRITE };


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FileTest, getTestSuiteName());
