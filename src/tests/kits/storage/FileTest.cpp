// FileTest.cpp

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestShell.h>

#include "FileTest.h"

// Suite
FileTest::Test*
FileTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<FileTest> TC;
	
	NodeTest::AddBaseClassTests<FileTest>("BFile::", suite);

	suite->addTest( new TC("BFile::Init Test 1", &FileTest::InitTest1) );
	suite->addTest( new TC("BFile::Init Test 2", &FileTest::InitTest2) );
	suite->addTest( new TC("BFile::IsRead-/IsWriteable Test",
						   &FileTest::RWAbleTest) );
	suite->addTest( new TC("BFile::Read/Write Test", &FileTest::RWTest) );
	suite->addTest( new TC("BFile::Position Test", &FileTest::PositionTest) );
	suite->addTest( new TC("BFile::Size Test", &FileTest::SizeTest) );
	suite->addTest( new TC("BFile::Assignment Test",
						   &FileTest::AssignmentTest) );
	return suite;
}		

// CreateRONodes
void
FileTest::CreateRONodes(TestNodes& testEntries)
{
	testEntries.clear();
	const char *filename;
	filename = existingFilename;
	testEntries.add(new BFile(filename, B_READ_ONLY), filename);
}

// CreateRWNodes
void
FileTest::CreateRWNodes(TestNodes& testEntries)
{
	testEntries.clear();
	const char *filename;
	filename = existingFilename;
	testEntries.add(new BFile(filename, B_READ_WRITE), filename);
}

// CreateUninitializedNodes
void
FileTest::CreateUninitializedNodes(TestNodes& testEntries)
{
	testEntries.clear();
	testEntries.add(new BFile, "");
}

// setUp
void FileTest::setUp()
{
	NodeTest::setUp();
}

// tearDown
void FileTest::tearDown()
{
	NodeTest::tearDown();
}

// InitTest1
void
FileTest::InitTest1()
{
	// 1. default constructor
	{
		BFile file;
		CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
	}

	// helper class for the testing the different constructors versions
	struct Tester {
		void testAll() const
		{
			for (int32 i = 0; i < initTestCasesCount; i++) {
				if (BTestShell::GlobalBeVerbose()) {
					printf("[%" B_PRId32 "]", i);
					fflush(stdout);
				}
				test(initTestCases[i]);
			}
			if (BTestShell::GlobalBeVerbose()) 
				printf("\n");
		}

		virtual void test(const InitTestCase& tc) const = 0;

		static void testInit(const InitTestCase& tc, BFile& file)
		{
			CPPUNIT_ASSERT( file.InitCheck() == tc.initCheck );
			if (tc.removeAfterTest)
				execCommand(string("rm ") + tc.filename);
		}
	};

	// 2. BFile(const char *, uint32)
	struct Tester1 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			BFile file(tc.filename,
					   tc.rwmode
					   | (tc.createFile * B_CREATE_FILE)
					   | (tc.failIfExists * B_FAIL_IF_EXISTS)
					   | (tc.eraseFile * B_ERASE_FILE));
			testInit(tc, file);
		}
	};

	// 3. BFile(entry_ref *, uint32)
	struct Tester2 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			entry_ref ref;
			BEntry entry(tc.filename);
			entry_ref *refToPass = &ref;
			if (tc.filename)
				CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
			else
				refToPass = NULL;
			BFile file(refToPass,
					   tc.rwmode
					   | (tc.createFile * B_CREATE_FILE)
					   | (tc.failIfExists * B_FAIL_IF_EXISTS)
					   | (tc.eraseFile * B_ERASE_FILE));
			testInit(tc, file);
		}
	};

	// 4. BFile(BEntry *, uint32)
	struct Tester3 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			entry_ref ref;
			BEntry entry(tc.filename);
			BEntry *entryToPass = &entry;
			if (tc.filename)
				CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
			else
				entryToPass = NULL;
			BFile file(entryToPass,
					   tc.rwmode
					   | (tc.createFile * B_CREATE_FILE)
					   | (tc.failIfExists * B_FAIL_IF_EXISTS)
					   | (tc.eraseFile * B_ERASE_FILE));
			testInit(tc, file);
		}
	};

	// 5. BFile(BEntry *, uint32)
	struct Tester4 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			if (tc.filename) {
				BPath path(tc.filename);
				CPPUNIT_ASSERT( path.InitCheck() == B_OK );
				BPath dirPath;
				CPPUNIT_ASSERT( path.GetParent(&dirPath) == B_OK );
				BDirectory dir(dirPath.Path());
				CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
				BFile file(&dir, path.Leaf(),
						   tc.rwmode
						   | (tc.createFile * B_CREATE_FILE)
						   | (tc.failIfExists * B_FAIL_IF_EXISTS)
						   | (tc.eraseFile * B_ERASE_FILE));
				testInit(tc, file);
			}
		}
	};

	Tester1().testAll();
	Tester2().testAll();
	Tester3().testAll();
	Tester4().testAll();
}

// InitTest2
void
FileTest::InitTest2()
{
	// helper class for the testing the different SetTo() versions
	struct Tester {
		void testAll() const
		{
			for (int32 i = 0; i < initTestCasesCount; i++) {
				if (BTestShell::GlobalBeVerbose()) {
					printf("[%" B_PRId32 "]", i);
					fflush(stdout);
				}
				test(initTestCases[i]);
			}
			if (BTestShell::GlobalBeVerbose()) 
				printf("\n");
		}

		virtual void test(const InitTestCase& tc) const = 0;

		static void testInit(const InitTestCase& tc, BFile& file)
		{
			CPPUNIT_ASSERT( file.InitCheck() == tc.initCheck );
			file.Unset();
			CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
			if (tc.removeAfterTest)
				execCommand(string("rm ") + tc.filename);
		}
	};

	// 2. BFile(const char *, uint32)
	struct Tester1 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			BFile file;
			status_t result = file.SetTo(tc.filename,
				tc.rwmode
				| (tc.createFile * B_CREATE_FILE)
				| (tc.failIfExists * B_FAIL_IF_EXISTS)
				| (tc.eraseFile * B_ERASE_FILE));
			CPPUNIT_ASSERT( result == tc.initCheck );
			testInit(tc, file);
		}
	};

	// 3. BFile(entry_ref *, uint32)
	struct Tester2 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			entry_ref ref;
			BEntry entry(tc.filename);
			entry_ref *refToPass = &ref;
			if (tc.filename)
				CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
			else
				refToPass = NULL;
			BFile file;
			status_t result = file.SetTo(refToPass,
				tc.rwmode
				| (tc.createFile * B_CREATE_FILE)
				| (tc.failIfExists * B_FAIL_IF_EXISTS)
				| (tc.eraseFile * B_ERASE_FILE));
			CPPUNIT_ASSERT( result == tc.initCheck );
			testInit(tc, file);
		}
	};

	// 4. BFile(BEntry *, uint32)
	struct Tester3 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			entry_ref ref;
			BEntry entry(tc.filename);
			BEntry *entryToPass = &entry;
			if (tc.filename)
				CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
			else
				entryToPass = NULL;
			BFile file;
			status_t result = file.SetTo(entryToPass,
				tc.rwmode
				| (tc.createFile * B_CREATE_FILE)
				| (tc.failIfExists * B_FAIL_IF_EXISTS)
				| (tc.eraseFile * B_ERASE_FILE));
			CPPUNIT_ASSERT( result == tc.initCheck );
			testInit(tc, file);
		}
	};

	// 5. BFile(BEntry *, uint32)
	struct Tester4 : public Tester {
		virtual void test(const InitTestCase& tc) const
		{
			if (tc.filename) {
				BPath path(tc.filename);
				CPPUNIT_ASSERT( path.InitCheck() == B_OK );
				BPath dirPath;
				CPPUNIT_ASSERT( path.GetParent(&dirPath) == B_OK );
				BDirectory dir(dirPath.Path());
				CPPUNIT_ASSERT( dir.InitCheck() == B_OK );
				BFile file;
				status_t result = file.SetTo(&dir, path.Leaf(),
					tc.rwmode
					| (tc.createFile * B_CREATE_FILE)
					| (tc.failIfExists * B_FAIL_IF_EXISTS)
					| (tc.eraseFile * B_ERASE_FILE));
				CPPUNIT_ASSERT( result == tc.initCheck );
				testInit(tc, file);
			}
		}
	};

	Tester1().testAll();
	Tester2().testAll();
	Tester3().testAll();
	Tester4().testAll();
}

// RWAbleTest
void
FileTest::RWAbleTest()
{
	NextSubTest();
	{
		BFile file;
		CPPUNIT_ASSERT( file.IsReadable() == false );
		CPPUNIT_ASSERT( file.IsWritable() == false );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT( file.IsReadable() == true );
		CPPUNIT_ASSERT( file.IsWritable() == false );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT( file.IsReadable() == false );
		CPPUNIT_ASSERT( file.IsWritable() == true );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT( file.IsReadable() == true );
		CPPUNIT_ASSERT( file.IsWritable() == true );
	}
	NextSubTest();
	{
		BFile file(nonExistingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT( file.IsReadable() == false );
		CPPUNIT_ASSERT( file.IsWritable() == false );
	}
}

// RWTest
void
FileTest::RWTest()
{
	// read/write an uninitialized BFile
	NextSubTest();
	BFile file;
	char buffer[10];
	CPPUNIT_ASSERT( file.Read(buffer, sizeof(buffer)) < 0 );
	CPPUNIT_ASSERT( file.ReadAt(0, buffer, sizeof(buffer)) < 0 );
	CPPUNIT_ASSERT( file.Write(buffer, sizeof(buffer)) < 0 );
	CPPUNIT_ASSERT( file.WriteAt(0, buffer, sizeof(buffer)) < 0 );
	file.Unset();
	// read/write an file opened for writing/reading only
	NextSubTest();
	file.SetTo(existingFilename, B_WRITE_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Read(buffer, sizeof(buffer)) < 0 );
	CPPUNIT_ASSERT( file.ReadAt(0, buffer, sizeof(buffer)) < 0 );
	file.SetTo(existingFilename, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Write(buffer, sizeof(buffer)) < 0 );
	CPPUNIT_ASSERT( file.WriteAt(0, buffer, sizeof(buffer)) < 0 );
	file.Unset();
	// read from an empty file
	NextSubTest();
	file.SetTo(existingFilename, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Read(buffer, sizeof(buffer)) == 0 );
	CPPUNIT_ASSERT( file.ReadAt(0, buffer, sizeof(buffer)) == 0 );
	file.Unset();
	// read from past an empty file
	NextSubTest();
	file.SetTo(existingFilename, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Seek(10, SEEK_SET) == 10 );
	CPPUNIT_ASSERT( file.Read(buffer, sizeof(buffer)) == 0 );
	CPPUNIT_ASSERT( file.ReadAt(10, buffer, sizeof(buffer)) == 0 );
	file.Unset();
	// create a new empty file and write some data into it, then
	// read the file and check the data
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	char writeBuffer[256];
	for (int32 i = 0; i < 256; i++)
		writeBuffer[i] = (char)i;
	CPPUNIT_ASSERT( file.Write(writeBuffer, 128) == 128 );
	CPPUNIT_ASSERT( file.Position() == 128 );
	CPPUNIT_ASSERT( file.Write(writeBuffer + 128, 128) == 128 );
	CPPUNIT_ASSERT( file.Position() == 256 );
	file.Unset();
	file.SetTo(testFilename1, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	char readBuffer[256];
	CPPUNIT_ASSERT( file.Read(readBuffer, 42) == 42 );
	CPPUNIT_ASSERT( file.Position() == 42 );
	CPPUNIT_ASSERT( file.Read(readBuffer + 42, 400) == 214 );
	CPPUNIT_ASSERT( file.Position() == 256 );
	for (int32 i = 0; i < 256; i++)
		CPPUNIT_ASSERT( readBuffer[i] == (char)i );
	file.Unset();
	execCommand(string("rm -f ") + testFilename1);
	// same procedure, just using ReadAt()/WriteAt()
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.WriteAt(80, writeBuffer + 80, 50) == 50 );
	CPPUNIT_ASSERT( file.Position() == 0 );
	CPPUNIT_ASSERT( file.WriteAt(0, writeBuffer, 80) == 80 );
	CPPUNIT_ASSERT( file.Position() == 0 );
	CPPUNIT_ASSERT( file.WriteAt(130, writeBuffer + 130, 126) == 126 );
	CPPUNIT_ASSERT( file.Position() == 0 );
	file.Unset();
	file.SetTo(testFilename1, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	for (int32 i = 0; i < 256; i++)
		readBuffer[i] = 0;
	CPPUNIT_ASSERT( file.ReadAt(42, readBuffer + 42, 84) == 84 );
	CPPUNIT_ASSERT( file.Position() == 0 );
	CPPUNIT_ASSERT( file.ReadAt(0, readBuffer, 42) == 42 );
	CPPUNIT_ASSERT( file.Position() == 0 );
	CPPUNIT_ASSERT( file.ReadAt(126, readBuffer + 126, 130) == 130 );
	CPPUNIT_ASSERT( file.Position() == 0 );
	for (int32 i = 0; i < 256; i++)
		CPPUNIT_ASSERT( readBuffer[i] == (char)i );
	file.Unset();
	execCommand(string("rm -f ") + testFilename1);
	// write past the end of a file
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Seek(128, SEEK_SET) == 128 );
	CPPUNIT_ASSERT( file.Write(writeBuffer, 128) == 128 );
	CPPUNIT_ASSERT( file.Position() == 256 );
	file.Unset();
	// open the file with B_OPEN_AT_END flag, Write() some data to it, close
	// and re-open it to check the file
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_OPEN_AT_END);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	for (int32 i = 0; i < 256; i++)
		writeBuffer[i] = (char)7;
	CPPUNIT_ASSERT( file.Write(writeBuffer, 50) == 50 );
	file.Seek(0, SEEK_SET);	// might fail -- don't check the return value
	CPPUNIT_ASSERT( file.Write(writeBuffer, 40) == 40 );
	file.Unset();
	file.SetTo(testFilename1, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.ReadAt(256, readBuffer, 90) == 90 );
	for (int32 i = 0; i < 90; i++)
		CPPUNIT_ASSERT( readBuffer[i] == 7 );
	file.Unset();
	// open the file with B_OPEN_AT_END flag, WriteAt() some data to it, close
	// and re-open it to check the file
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_OPEN_AT_END);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	for (int32 i = 0; i < 256; i++)
		writeBuffer[i] = (char)42;
	CPPUNIT_ASSERT( file.WriteAt(0, writeBuffer, 30) == 30 );
	file.Unset();
	file.SetTo(testFilename1, B_READ_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.ReadAt(346, readBuffer, 30) == 30 );
	for (int32 i = 0; i < 30; i++)
		CPPUNIT_ASSERT( readBuffer[i] == 42 );
	file.Unset();
	// open the file with B_OPEN_AT_END flag, ReadAt() some data
	NextSubTest();
	file.SetTo(testFilename1, B_READ_ONLY | B_OPEN_AT_END);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	for (int32 i = 0; i < 256; i++)
		readBuffer[i] = 0;
	CPPUNIT_ASSERT( file.ReadAt(256, readBuffer, 90) == 90 );
	for (int32 i = 0; i < 90; i++)
		CPPUNIT_ASSERT( readBuffer[i] == 7 );
	CPPUNIT_ASSERT( file.ReadAt(346, readBuffer, 30) == 30 );
	for (int32 i = 0; i < 30; i++)
		CPPUNIT_ASSERT( readBuffer[i] == 42 );
	file.Unset();
	// same procedure, just using Seek() and Read()
	NextSubTest();
	file.SetTo(testFilename1, B_READ_ONLY | B_OPEN_AT_END);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	for (int32 i = 0; i < 256; i++)
		readBuffer[i] = 0;
	file.Seek(256, SEEK_SET);	// might fail -- don't check the return value
	CPPUNIT_ASSERT( file.Read(readBuffer, 90) == 90 );
	for (int32 i = 0; i < 90; i++)
		CPPUNIT_ASSERT( readBuffer[i] == 7 );
	CPPUNIT_ASSERT( file.ReadAt(346, readBuffer, 30) == 30 );
	for (int32 i = 0; i < 30; i++)
		CPPUNIT_ASSERT( readBuffer[i] == 42 );
	file.Unset();

	execCommand(string("rm -f ") + testFilename1);
}

// PositionTest
void
FileTest::PositionTest()
{
	// unitialized file
	NextSubTest();
	BFile file;
	CPPUNIT_ASSERT( file.Position() == B_FILE_ERROR );
	CPPUNIT_ASSERT( file.Seek(10, SEEK_SET) == B_FILE_ERROR );
	CPPUNIT_ASSERT( file.Seek(10, SEEK_END) == B_FILE_ERROR );
	CPPUNIT_ASSERT( file.Seek(10, SEEK_CUR) == B_FILE_ERROR );
	// open new file, write some bytes to it and seek a bit around
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Position() == 0 );
	char writeBuffer[256];
	CPPUNIT_ASSERT( file.Write(writeBuffer, 256) == 256 );
	CPPUNIT_ASSERT( file.Position() == 256 );
	CPPUNIT_ASSERT( file.Seek(10, SEEK_SET) == 10 );
	CPPUNIT_ASSERT( file.Position() == 10 );
	CPPUNIT_ASSERT( file.Seek(-20, SEEK_END) == 236 );
	CPPUNIT_ASSERT( file.Position() == 236 );
	CPPUNIT_ASSERT( file.Seek(-70, SEEK_CUR) == 166 );
	CPPUNIT_ASSERT( file.Position() == 166 );
	file.Unset();
	// re-open the file at the end and seek a bit around once more
	// The BeBook is a bit unspecific about the B_OPEN_AT_END flag:
	// It has probably the same meaning as the POSIX flag O_APPEND, which
	// means, that all write()s append their data at the end. The behavior
	// of Seek() and Position() is a bit unclear for this case.
/*
	NextSubTest();
	file.SetTo(testFilename1, B_READ_ONLY | B_OPEN_AT_END);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.Position() == 256 );
	CPPUNIT_ASSERT( file.Seek(10, SEEK_SET) == 10 );
	CPPUNIT_ASSERT( file.Position() == 10 );			// fails with R5
	CPPUNIT_ASSERT( file.Seek(-20, SEEK_END) == 236 );
	CPPUNIT_ASSERT( file.Position() == 236 );			// fails with R5
	CPPUNIT_ASSERT( file.Seek(-70, SEEK_CUR) == 166 );	// fails with R5
	CPPUNIT_ASSERT( file.Position() == 166 );			// fails with R5
*/
	file.Unset();
	execCommand(string("rm -f ") + testFilename1);
}

// SizeTest
void
FileTest::SizeTest()
{
	// unitialized file
	NextSubTest();
	BFile file;
	off_t size;
	CPPUNIT_ASSERT( file.GetSize(&size) != B_OK );
	CPPUNIT_ASSERT( file.SetSize(100) != B_OK );
	// read only file, SetSize will not succeed
	NextSubTest();
	file.SetTo(testFilename1, B_READ_ONLY | B_CREATE_FILE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 0 );
	CPPUNIT_ASSERT_EQUAL(file.SetSize(100), B_BAD_VALUE);
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT_EQUAL(size, 0);
	file.Unset();
	// successfully set size of file with appropriate flags
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT_EQUAL(size, 0);
	CPPUNIT_ASSERT( file.SetSize(73) == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 73 );
	file.Unset();
	// enlarge existing file
	NextSubTest();
	file.SetTo(testFilename1, B_READ_WRITE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 73 );
	CPPUNIT_ASSERT( file.SetSize(147) == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 147 );
	file.Unset();
	// erase existing file (write only)
	NextSubTest();
	file.SetTo(testFilename1, B_WRITE_ONLY | B_ERASE_FILE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 0 );
	CPPUNIT_ASSERT( file.SetSize(93) == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 93 );
	file.Unset();
	// erase existing file using SetSize()
	NextSubTest();
	file.SetTo(testFilename1, B_READ_WRITE);
	CPPUNIT_ASSERT( file.InitCheck() == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 93 );
	CPPUNIT_ASSERT( file.SetSize(0) == B_OK );
	CPPUNIT_ASSERT( file.GetSize(&size) == B_OK );
	CPPUNIT_ASSERT( size == 0 );
	file.Unset();
	execCommand(string("rm -f ") + testFilename1);
}

// AssignmentTest
void
FileTest::AssignmentTest()
{
	// copy constructor
	// uninitialized
	NextSubTest();
	{
		BFile file;
		CPPUNIT_ASSERT( file.InitCheck() == B_NO_INIT );
		BFile file2(file);
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(file2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	// existing file, different open modes
	NextSubTest();
	{
		BFile file(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BFile file2(file);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( file2.IsReadable() == true );
		CPPUNIT_ASSERT( file2.IsWritable() == false );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BFile file2(file);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( file2.IsReadable() == false );
		CPPUNIT_ASSERT( file2.IsWritable() == true );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BFile file2(file);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( file2.IsReadable() == true );
		CPPUNIT_ASSERT( file2.IsWritable() == true );
	}
	// assignment operator
	// uninitialized
	NextSubTest();
	{
		BFile file;
		BFile file2;
		file2 = file;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(file2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	NextSubTest();
	{
		BFile file;
		BFile file2(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		file2 = file;
		// R5 returns B_BAD_VALUE instead of B_NO_INIT
		CPPUNIT_ASSERT( equals(file2.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	// existing file, different open modes
	NextSubTest();
	{
		BFile file(existingFilename, B_READ_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BFile file2;
		file2 = file;
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( file2.IsReadable() == true );
		CPPUNIT_ASSERT( file2.IsWritable() == false );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_WRITE_ONLY);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BFile file2;
		file2 = file;
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( file2.IsReadable() == false );
		CPPUNIT_ASSERT( file2.IsWritable() == true );
	}
	NextSubTest();
	{
		BFile file(existingFilename, B_READ_WRITE);
		CPPUNIT_ASSERT( file.InitCheck() == B_OK );
		BFile file2;
		file2 = file;
		CPPUNIT_ASSERT( file2.InitCheck() == B_OK );
		CPPUNIT_ASSERT( file2.IsReadable() == true );
		CPPUNIT_ASSERT( file2.IsWritable() == true );
	}
}



// test cases for the init tests
const FileTest::InitTestCase FileTest::initTestCases[] = {
	{ existingFilename	 , B_READ_ONLY , 0, 0, 0, false, B_OK				},
	{ existingFilename	 , B_WRITE_ONLY, 0, 0, 0, false, B_OK				},
	{ existingFilename	 , B_READ_WRITE, 0, 0, 0, false, B_OK				},
	{ existingFilename	 , B_READ_ONLY , 1, 0, 0, false, B_OK				},
	{ existingFilename	 , B_WRITE_ONLY, 1, 0, 0, false, B_OK				},
	{ existingFilename	 , B_READ_WRITE, 1, 0, 0, false, B_OK				},
	{ existingFilename	 , B_READ_ONLY , 0, 1, 0, false, B_OK				},
	{ existingFilename	 , B_WRITE_ONLY, 0, 1, 0, false, B_OK				},
	{ existingFilename	 , B_READ_WRITE, 0, 1, 0, false, B_OK				},
	{ existingFilename	 , B_READ_ONLY , 0, 0, 1, false, B_NOT_ALLOWED		},
	{ existingFilename	 , B_WRITE_ONLY, 0, 0, 1, false, B_OK				},
	{ existingFilename	 , B_READ_WRITE, 0, 0, 1, false, B_OK				},
	{ existingFilename	 , B_READ_ONLY , 1, 1, 0, false, B_FILE_EXISTS		},
	{ existingFilename	 , B_WRITE_ONLY, 1, 1, 0, false, B_FILE_EXISTS		},
	{ existingFilename	 , B_READ_WRITE, 1, 1, 0, false, B_FILE_EXISTS		},
	{ existingFilename	 , B_READ_ONLY , 1, 0, 1, false, B_NOT_ALLOWED		},
	{ existingFilename	 , B_WRITE_ONLY, 1, 0, 1, false, B_OK				},
	{ existingFilename	 , B_READ_WRITE, 1, 0, 1, false, B_OK				},
	{ existingFilename	 , B_READ_ONLY , 1, 1, 1, false, B_FILE_EXISTS		},
	{ existingFilename	 , B_WRITE_ONLY, 1, 1, 1, false, B_FILE_EXISTS		},
	{ existingFilename	 , B_READ_WRITE, 1, 1, 1, false, B_FILE_EXISTS		},
	{ nonExistingFilename, B_READ_ONLY , 0, 0, 0, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_WRITE_ONLY, 0, 0, 0, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_READ_WRITE, 0, 0, 0, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_READ_ONLY , 1, 0, 0, true , B_OK				},
	{ nonExistingFilename, B_WRITE_ONLY, 1, 0, 0, true , B_OK				},
	{ nonExistingFilename, B_READ_WRITE, 1, 0, 0, true , B_OK				},
	{ nonExistingFilename, B_READ_ONLY , 0, 1, 0, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_WRITE_ONLY, 0, 1, 0, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_READ_WRITE, 0, 1, 0, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_READ_ONLY , 0, 0, 1, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_WRITE_ONLY, 0, 0, 1, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_READ_WRITE, 0, 0, 1, false, B_ENTRY_NOT_FOUND	},
	{ nonExistingFilename, B_READ_ONLY , 1, 1, 0, true , B_OK				},
	{ nonExistingFilename, B_WRITE_ONLY, 1, 1, 0, true , B_OK				},
	{ nonExistingFilename, B_READ_WRITE, 1, 1, 0, true , B_OK				},
	{ nonExistingFilename, B_READ_ONLY , 1, 0, 1, true , B_OK				},
	{ nonExistingFilename, B_WRITE_ONLY, 1, 0, 1, true , B_OK				},
	{ nonExistingFilename, B_READ_WRITE, 1, 0, 1, true , B_OK				},
	{ nonExistingFilename, B_READ_ONLY , 1, 1, 1, true , B_OK				},
	{ nonExistingFilename, B_WRITE_ONLY, 1, 1, 1, true , B_OK				},
	{ nonExistingFilename, B_READ_WRITE, 1, 1, 1, true , B_OK				},
	{ NULL,				   B_READ_ONLY , 1, 1, 1, false, B_BAD_VALUE		},
};
const int32 FileTest::initTestCasesCount
	= sizeof(FileTest::initTestCases) / sizeof(FileTest::InitTestCase);











