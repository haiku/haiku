/*
 * Copyright 2026 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <File.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "FileReadWrite.h"


class FileReadWriteTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(FileReadWriteTest);
	CPPUNIT_TEST(ReadFile_WithMultipleLines);
	CPPUNIT_TEST(ReadFile_WithoutAnyNewLines);
	CPPUNIT_TEST(ReadFile_WithLinesLongerThanBufferSize);
	CPPUNIT_TEST(ReadFile_WithEmptyLines);
	CPPUNIT_TEST(Read_EmptyFile);
	CPPUNIT_TEST_SUITE_END();

	void _TestReadGeneric(BString input, BString& output) {
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

public:
	void Read_EmptyFile()
	{
		const BString content = "";
		BString output;

		_TestReadGeneric(content, output);

		CPPUNIT_ASSERT_EQUAL(content, output);
	}

	void ReadFile_WithEmptyLines()
	{
		const BString content = "Line 1\n\nLine2\n\n\nLine3\n\n";
		BString output;

		_TestReadGeneric(content, output);

		CPPUNIT_ASSERT_EQUAL(content, output);
	}

	void ReadFile_WithoutAnyNewLines()
	{
		const BString content = "Line 1";
		BString output;

		_TestReadGeneric(content, output);

		CPPUNIT_ASSERT_EQUAL(content, output);
	}

	void ReadFile_WithLinesLongerThanBufferSize()
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

		_TestReadGeneric(content, output);

		CPPUNIT_ASSERT_EQUAL(content, output);
	}

	void ReadFile_WithMultipleLines()
	{
		const BString content = "Line 1\nLine 2\nLine 3\n";
		BString output;

		_TestReadGeneric(content, output);

		CPPUNIT_ASSERT_EQUAL(content, output);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(FileReadWriteTest, getTestSuiteName());
