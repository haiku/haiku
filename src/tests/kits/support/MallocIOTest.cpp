/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <DataIO.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string.h>


class MallocIOTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MallocIOTest);
	CPPUNIT_TEST(BufferLength_Initial_IsZero);
	CPPUNIT_TEST(BufferLength_AfterWrite_IsCorrect);
	CPPUNIT_TEST(BufferLength_AfterSetSizeZero_IsZero);
	CPPUNIT_TEST(BufferLength_AfterSetSizeIncrease_IsCorrectAndSeekEndIsCorrect);
	CPPUNIT_TEST(BufferLength_AfterSetSizeDecrease_IsCorrectAndPositionIsUnchanged);

	CPPUNIT_TEST(Seek_Set_ReturnsExpectedPosition);
	CPPUNIT_TEST(Seek_Cur_ReturnsExpectedPosition);
	CPPUNIT_TEST(Seek_End_ReturnsExpectedPosition);
	CPPUNIT_TEST(Seek_EndNegative_ReturnsParsedPosition);
	CPPUNIT_TEST(Seek_EndPositive_ReturnsPositionOutOfBounds);
	CPPUNIT_TEST(Seek_SetNegative_ReturnsParsedPosition);

	CPPUNIT_TEST(Write_Normal_ReturnsWrittenLength);
	CPPUNIT_TEST(WriteAt_ZeroOffset_ReturnsWrittenLength);
	CPPUNIT_TEST(WriteAt_LargeOffset_ReturnsWrittenLengthAndExpandsBuffer);
	CPPUNIT_TEST_SUITE_END();

public:
	void BufferLength_Initial_IsZero()
	{
		size_t bufLen = fMem.BufferLength();
		CPPUNIT_ASSERT_EQUAL((size_t)0, bufLen);
	}

	void BufferLength_AfterWrite_IsCorrect()
	{
		char writeBuf[11] = "0123456789";
		ssize_t size = fMem.Write(writeBuf, 10);
		size_t bufLen = fMem.BufferLength();
		CPPUNIT_ASSERT_EQUAL((size_t)10, bufLen);
		CPPUNIT_ASSERT_EQUAL((ssize_t)10, size);
	}

	void BufferLength_AfterSetSizeZero_IsZero()
	{
		status_t error = fMem.SetSize(0);
		size_t bufLen = fMem.BufferLength();
		CPPUNIT_ASSERT_EQUAL((size_t)0, bufLen);
		CPPUNIT_ASSERT_EQUAL(B_OK, error);
	}

	void BufferLength_AfterSetSizeIncrease_IsCorrectAndSeekEndIsCorrect()
	{
		status_t error = fMem.SetSize(200);
		size_t bufLen = fMem.BufferLength();
		off_t offset = fMem.Seek(0, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((size_t)200, bufLen);
		CPPUNIT_ASSERT_EQUAL(B_OK, error);
		CPPUNIT_ASSERT_EQUAL((off_t)200, offset);
	}

	void BufferLength_AfterSetSizeDecrease_IsCorrectAndPositionIsUnchanged()
	{
		fMem.SetSize(200);
		off_t offset = fMem.Seek(0, SEEK_END);
		status_t error = fMem.SetSize(100);
		size_t bufLen = fMem.BufferLength();
		CPPUNIT_ASSERT_EQUAL((size_t)100, bufLen);
		CPPUNIT_ASSERT_EQUAL(offset, fMem.Position());
		CPPUNIT_ASSERT_EQUAL(B_OK, error);
	}

	void Seek_Set_ReturnsExpectedPosition()
	{
		off_t err = fMem.Seek(3, SEEK_SET);
		CPPUNIT_ASSERT_EQUAL((off_t)3, err);
	}

	void Seek_Cur_ReturnsExpectedPosition()
	{
		off_t err = fMem.Seek(3, SEEK_CUR);
		CPPUNIT_ASSERT_EQUAL((off_t)3, err);
	}

	void Seek_End_ReturnsExpectedPosition()
	{
		off_t err = fMem.Seek(0, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((off_t)0, err);
	}

	void Seek_EndNegative_ReturnsParsedPosition()
	{
		off_t err = fMem.Seek(-5, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((off_t)-5, err);
	}

	void Seek_EndPositive_ReturnsPositionOutOfBounds()
	{
		off_t err = fMem.Seek(5, SEEK_END);
		CPPUNIT_ASSERT_EQUAL((off_t)5, err);
	}

	void Seek_SetNegative_ReturnsParsedPosition()
	{
		off_t err = fMem.Seek(-20, SEEK_SET);
		CPPUNIT_ASSERT_EQUAL((off_t)-20, err);
	}

	void Write_Normal_ReturnsWrittenLength()
	{
		const char* writeBuf = "ABCDEFG";
		ssize_t err = fMem.Write(writeBuf, 7);
		CPPUNIT_ASSERT_EQUAL((ssize_t)7, err);
	}

	void WriteAt_ZeroOffset_ReturnsWrittenLength()
	{
		const char* writeBuf = "ABCDEFG";
		ssize_t err = fMem.WriteAt(0, writeBuf, 4);
		CPPUNIT_ASSERT_EQUAL((ssize_t)4, err);
	}

	void WriteAt_LargeOffset_ReturnsWrittenLengthAndExpandsBuffer()
	{
		const char* writeBuf = "ABCDEFG";
		ssize_t err = fMem.WriteAt(34, writeBuf, 256);
		CPPUNIT_ASSERT_EQUAL((ssize_t)256, err);
	}

private:
	BMallocIO fMem;
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MallocIOTest, getTestSuiteName());
