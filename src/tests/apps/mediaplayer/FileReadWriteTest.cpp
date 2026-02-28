#include "FileReadWriteTest.h"

#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "FileReadWrite.h"


FileReadWriteTest::FileReadWriteTest()
{
}


FileReadWriteTest::~FileReadWriteTest()
{
}


void
FileReadWriteTest::TestReadFileWithMultipleLines()
{
	const BString content = "Line 1\nLine 2\nLine 3\n";
	BString output;

// ----------------------
	TestReadGeneric(content, output);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(content, output);
}


void
FileReadWriteTest::TestReadFileWithLinesLongerThanBufferSize()
{
	BString content;
	BString line;
	int32 high = 8192, low = 4096;
	for (int32 i = 0; i < 2; i++) {
		int32 randomLength = rand() % (high - low + 1) + low;
		line.SetTo('a', randomLength);
		content += line;
		content += '\n';
	}

	BString output;

// ----------------------
	TestReadGeneric(content, output);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(content, output);
}


void
FileReadWriteTest::TestReadFileWithoutAnyNewLines()
{
	const BString content = "Line 1";
	BString output;

// ----------------------
	TestReadGeneric(content, output);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(content, output);
}


void
FileReadWriteTest::TestReadFileWithEmptyLines()
{
	const BString content = "Line 1\n\nLine2\n\n\nLine3\n\n";
	BString output;

// ----------------------
	TestReadGeneric(content, output);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(content, output);
}


void
FileReadWriteTest::TestReadEmptyFile()
{
	const BString content = "";
	BString output;

// ----------------------
	TestReadGeneric(content, output);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(content, output);
}


void
FileReadWriteTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("FileReadWriteTest");

	suite.addTest(
		new CppUnit::TestCaller<FileReadWriteTest>(
			"FileReadWriteTest::TestReadFileWithMultipleLines",
			&FileReadWriteTest::TestReadFileWithMultipleLines));
	suite.addTest(
		new CppUnit::TestCaller<FileReadWriteTest>(
			"FileReadWriteTest::TestReadFileWithLinesLongerThanBufferSize",
			&FileReadWriteTest::TestReadFileWithLinesLongerThanBufferSize));
	suite.addTest(
		new CppUnit::TestCaller<FileReadWriteTest>(
			"FileReadWriteTest::TestReadFileWithoutAnyNewLines",
			&FileReadWriteTest::TestReadFileWithoutAnyNewLines));
	suite.addTest(
		new CppUnit::TestCaller<FileReadWriteTest>(
			"FileReadWriteTest::TestReadFileWithEmptyLines",
			&FileReadWriteTest::TestReadFileWithEmptyLines));
	suite.addTest(
		new CppUnit::TestCaller<FileReadWriteTest>(
			"FileReadWriteTest::TestReadEmptyFile",
			&FileReadWriteTest::TestReadEmptyFile));

	parent.addTest("FileReadWriteTest", &suite);
}


void
FileReadWriteTest::TestReadGeneric(BString input, BString& output) {
	// use current folder if can't find temp directory
	BPath path;
	if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &path) != B_NO_ERROR)
		path = "./";

	// generate the test file
	BFile fWrite;
	BString fPath(path.Path());
	fPath << "TempTestFile" << system_time() << ".txt";

	fWrite.SetTo(fPath, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	fWrite.Write(input, input.Length());

	// reading file's contents then comparing it to the real data
	BFile fRead(fPath, B_READ_ONLY);
	FileReadWrite reader(&fRead);
	BString line;

	while (reader.Next(line)) {
		output += line.String();
		output += '\n';
		line.Truncate(0);
	}
	output += line.String();

	BEntry entry(fPath);
	entry.Remove();
}
