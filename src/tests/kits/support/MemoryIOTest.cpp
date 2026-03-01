/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <DataIO.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string.h>


class MemoryIOTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MemoryIOTest);
	CPPUNIT_TEST(ReadOnlyMemory_Write_ReturnsNotAllowed);
	CPPUNIT_TEST(ReadOnlyMemory_WriteAt_ReturnsNotAllowed);
	CPPUNIT_TEST(ReadOnlyMemory_SetSize_Smaller_ReturnsNotAllowed);
	CPPUNIT_TEST(ReadOnlyMemory_SetSize_Larger_ReturnsNotAllowed);

	CPPUNIT_TEST(Read_Normal_Succeeds);
	CPPUNIT_TEST(ReadAt_OutOfBounds_ReturnsZero);
	CPPUNIT_TEST(Read_AtEOF_ReturnsZero);

	CPPUNIT_TEST(Seek_Set_ReturnsExpectedPosition);
	CPPUNIT_TEST(Seek_Cur_ReturnsExpectedPosition);
	CPPUNIT_TEST(Seek_End_ReturnsExpectedPosition);
	CPPUNIT_TEST(Seek_EndNegative_ReturnsParsedPosition);
	CPPUNIT_TEST(Seek_EndPositive_ReturnsPositionOutOfBounds);

	CPPUNIT_TEST(SetSize_Smaller_TruncatesAndReturnsOK);
	CPPUNIT_TEST(SetSize_Same_ReturnsOK);
	CPPUNIT_TEST(SetSize_Larger_ReturnsError);

	CPPUNIT_TEST(Write_Normal_Succeeds);
	CPPUNIT_TEST(WriteAt_Normal_Succeeds);
	CPPUNIT_TEST(WriteAt_Truncated_Succeeds);
	CPPUNIT_TEST(WriteAt_NegativeOffset_ReturnsBadValue);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp()
	{
		fBufferSize = 20;
		fBuffer = new char[fBufferSize];
		memcpy(fBuffer, "0123456789ABCDEFGHI", fBufferSize);
		fReadBuffer = new char[10];
		memset(fReadBuffer, 0, 10);

		fMem = new BMemoryIO(fBuffer, fBufferSize);
		fReadOnlyMem = new BMemoryIO((const void*)fBuffer, fBufferSize);
	}

	void tearDown()
	{
		delete fReadOnlyMem;
		delete fMem;
		delete[] fReadBuffer;
		delete[] fBuffer;
	}

	void ReadOnlyMemory_Write_ReturnsNotAllowed()
	{
		status_t err = fReadOnlyMem->Write(fReadBuffer, 3);
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, err);
	}

	void ReadOnlyMemory_WriteAt_ReturnsNotAllowed()
	{
		const char* writeBuf = "ABCDEFG";
		off_t pos = fReadOnlyMem->Position();
		status_t err = fReadOnlyMem->WriteAt(2, writeBuf, 1);
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, err);
		CPPUNIT_ASSERT_EQUAL(pos, fReadOnlyMem->Position());
	}

	void ReadOnlyMemory_SetSize_Smaller_ReturnsNotAllowed()
	{
		status_t err = fReadOnlyMem->SetSize(4);
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, err);
	}

	void ReadOnlyMemory_SetSize_Larger_ReturnsNotAllowed()
	{
		status_t err = fReadOnlyMem->SetSize(40);
		CPPUNIT_ASSERT_EQUAL(B_NOT_ALLOWED, err);
	}

	void Read_Normal_Succeeds()
	{
		off_t pos = fMem->Position();
		ssize_t err = fMem->Read(fReadBuffer, 10);
		CPPUNIT_ASSERT_EQUAL((ssize_t)10, err);
		CPPUNIT_ASSERT(strncmp(fReadBuffer, fBuffer, 10) == 0);
		CPPUNIT_ASSERT_EQUAL(pos + err, fMem->Position());
	}

	void ReadAt_OutOfBounds_ReturnsZero()
	{
		off_t pos = fMem->Position();
		ssize_t err = fMem->ReadAt(30, fReadBuffer, 10);
		CPPUNIT_ASSERT_EQUAL((ssize_t)0, err);
		CPPUNIT_ASSERT_EQUAL(pos, fMem->Position());
	}

	void Read_AtEOF_ReturnsZero()
	{
		off_t pos = fMem->Seek(0, SEEK_END);
		ssize_t err = fMem->Read(fReadBuffer, 10);
		CPPUNIT_ASSERT_EQUAL((ssize_t)0, err);
		CPPUNIT_ASSERT_EQUAL(pos, fMem->Position());
	}

	void Seek_Set_ReturnsExpectedPosition()
	{
		off_t err = fMem->Seek(3, SEEK_SET);
		CPPUNIT_ASSERT_EQUAL((off_t)3, err);
	}

	void Seek_Cur_ReturnsExpectedPosition()
	{
		off_t err = fMem->Seek(3, SEEK_CUR);
		CPPUNIT_ASSERT_EQUAL((off_t)3, err);
	}

	void Seek_End_ReturnsExpectedPosition()
	{
		off_t err = fMem->Seek(0, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((off_t)fBufferSize, err);
	}

	void Seek_EndNegative_ReturnsParsedPosition()
	{
		off_t err = fMem->Seek(-5, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((off_t)fBufferSize - 5, err);
	}

	void Seek_EndPositive_ReturnsPositionOutOfBounds()
	{
		off_t err = fMem->Seek(5, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((off_t)fBufferSize + 5, err);
	}

	void SetSize_Smaller_TruncatesAndReturnsOK()
	{
		BMemoryIO mem(fBuffer, 10);
		status_t err = mem.SetSize(5);
		CPPUNIT_ASSERT_EQUAL(B_OK, err);
		CPPUNIT_ASSERT_EQUAL((off_t)5, mem.Seek(0, SEEK_END));

		ssize_t size = mem.WriteAt(10, fReadBuffer, 3);
		CPPUNIT_ASSERT_EQUAL((ssize_t)0, size);
	}

	void SetSize_Same_ReturnsOK()
	{
		BMemoryIO mem(fBuffer, 10);
		status_t err = mem.SetSize(10);
		CPPUNIT_ASSERT_EQUAL(B_OK, err);
		CPPUNIT_ASSERT_EQUAL((off_t)10, mem.Seek(0, SEEK_END));

		ssize_t size = mem.WriteAt(5, fReadBuffer, 6);
		CPPUNIT_ASSERT_EQUAL((ssize_t)5, size);
	}

	void SetSize_Larger_ReturnsError()
	{
		BMemoryIO mem(fBuffer, 10);
		status_t err = mem.SetSize(20);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, err);
	}

	void Write_Normal_Succeeds()
	{
		const char* writeBuf = "ABCDEFG";
		off_t pos = fMem->Position();
		ssize_t err = fMem->Write(writeBuf, 7);
		CPPUNIT_ASSERT_EQUAL((ssize_t)7, err);
		CPPUNIT_ASSERT(strncmp(writeBuf, fBuffer, 7) == 0);
		CPPUNIT_ASSERT_EQUAL(pos + err, fMem->Position());
	}

	void WriteAt_Normal_Succeeds()
	{
		const char* writeBuf = "ABCDEFG";
		off_t pos = fMem->Position();
		ssize_t err = fMem->WriteAt(3, writeBuf, 2);
		CPPUNIT_ASSERT_EQUAL((ssize_t)2, err);
		CPPUNIT_ASSERT(strncmp(fBuffer + 3, writeBuf, 2) == 0);
		CPPUNIT_ASSERT_EQUAL(pos, fMem->Position());
	}

	void WriteAt_Truncated_Succeeds()
	{
		const char* writeBuf = "ABCDEFG";
		off_t pos = fMem->Position();
		ssize_t err = fMem->WriteAt(fBufferSize - 1, writeBuf, 5);
		CPPUNIT_ASSERT_EQUAL((ssize_t)1, err);
		CPPUNIT_ASSERT(strncmp(fBuffer + fBufferSize - 1, writeBuf, 1) == 0);
		CPPUNIT_ASSERT_EQUAL(pos, fMem->Position());
	}

	void WriteAt_NegativeOffset_ReturnsBadValue()
	{
		const char* writeBuf = "ABCDEFG";
		off_t pos = fMem->Position();
		ssize_t err = fMem->WriteAt(-10, writeBuf, 5);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, err);
		CPPUNIT_ASSERT_EQUAL(pos, fMem->Position());
	}

private:
	char* fBuffer;
	size_t fBufferSize;
	char* fReadBuffer;
	BMemoryIO* fMem;
	BMemoryIO* fReadOnlyMem;
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MemoryIOTest, getTestSuiteName());
