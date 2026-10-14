/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Matzon <brian@matzon.dk>
 *		Michael Wilber
 */


#include <Application.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <TranslatorRoster.h>

#include <stdio.h>
#include <string.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class BitmapStreamTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(BitmapStreamTest);
	CPPUNIT_TEST(Default_Constructor_InitializesToZero);
	CPPUNIT_TEST(Bitmap_Constructor_InitializesWithBitmap);
	CPPUNIT_TEST(TranslatedFile_DetachBitmap_Succeeds);
	CPPUNIT_TEST(Null_DetachBitmap_ReturnsBadValue);
	CPPUNIT_TEST(Bitmap_DetachBitmap_SecondDetachReturnsError);
	CPPUNIT_TEST(BitmapDetachedFromStream_DeleteStream_PreservesBitmap);
	CPPUNIT_TEST(StreamWithAttachedBitmap_DeleteStream_DestroysBitmap);
	CPPUNIT_TEST(Header_ReadWriteByteByByte_Succeeds);
	CPPUNIT_TEST(Header_ReadWriteAllAtOnce_Succeeds);
	CPPUNIT_TEST(BitmapData_ReadWrite_Succeeds);
	CPPUNIT_TEST(ZeroBytes_ReadWrite_Succeeds);
	CPPUNIT_TEST(ErroneousData_ReadWrite_ReturnsErrors);
	CPPUNIT_TEST(SEEK_END_Seek_Succeeds);
	CPPUNIT_TEST(SEEK_SET_Seek_Succeeds);
	CPPUNIT_TEST(SEEK_CUR_Seek_Succeeds);
	CPPUNIT_TEST(InvalidValues_SetSize_ReturnsBadValue);
	CPPUNIT_TEST_SUITE_END();

public:
	void Default_Constructor_InitializesToZero()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BBitmapStream streamEmpty;
		BBitmap* pbits = NULL;
		CPPUNIT_ASSERT(streamEmpty.Position() == 0);
		CPPUNIT_ASSERT(streamEmpty.Size() == 0);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, streamEmpty.DetachBitmap(&pbits));
		CPPUNIT_ASSERT(pbits == NULL);
	}

	void Bitmap_Constructor_InitializesWithBitmap()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BBitmap* pbits = new BBitmap(BRect(0, 0, 5, 5), B_RGB32);
		CPPUNIT_ASSERT(pbits);

		BBitmapStream* pstreamWithBits;
		pstreamWithBits = new BBitmapStream(pbits);
		CPPUNIT_ASSERT(pstreamWithBits);
		CPPUNIT_ASSERT(pstreamWithBits->Position() == 0);
		CPPUNIT_ASSERT(pstreamWithBits->Size() == 176);
		BBitmap* poutbits = NULL;
		CPPUNIT_ASSERT_EQUAL(B_OK, pstreamWithBits->DetachBitmap(&poutbits));
		CPPUNIT_ASSERT(pbits == poutbits);

		delete pstreamWithBits;
		pstreamWithBits = NULL;

		delete pbits;
		pbits = NULL;
	}

	void TranslatedFile_DetachBitmap_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BFile file("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());

		BBitmapStream* pstream = new BBitmapStream;
		BBitmap* pbits = NULL;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&file, NULL, NULL, pstream, B_TRANSLATOR_BITMAP));
		CPPUNIT_ASSERT_EQUAL(B_NO_ERROR, pstream->DetachBitmap(&pbits));
		CPPUNIT_ASSERT(pbits);
		CPPUNIT_ASSERT(pbits->IsValid());

		delete pstream;
		delete pbits;
	}

	void Null_DetachBitmap_ReturnsBadValue()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BBitmapStream* pstream = new BBitmapStream;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, pstream->DetachBitmap(NULL));
		delete pstream;
	}

	void Bitmap_DetachBitmap_SecondDetachReturnsError()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BBitmapStream* pstream = new BBitmapStream;
		BBitmap* pbits = NULL;
		BBitmap* pbits2 = NULL;

		// to enable detach, we need a bitmap
		pbits = new BBitmap(BRect(0, 0, 5, 5), B_RGB32);
		delete pstream;
		pstream = new BBitmapStream(pbits);

		CPPUNIT_ASSERT_EQUAL(B_OK, pstream->DetachBitmap(&pbits2));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, pstream->DetachBitmap(&pbits2));

		delete pstream;
		delete pbits;
	}

	void BitmapDetachedFromStream_DeleteStream_PreservesBitmap()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BFile file("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());

		BBitmapStream* pstream = new BBitmapStream;
		BBitmap* pbits = NULL;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&file, NULL, NULL, pstream, B_TRANSLATOR_BITMAP));
		CPPUNIT_ASSERT_EQUAL(B_NO_ERROR, pstream->DetachBitmap(&pbits));
		CPPUNIT_ASSERT(pbits);

		delete pstream;
		pstream = NULL;
		CPPUNIT_ASSERT(pbits->IsValid());
		CPPUNIT_ASSERT(pbits->BitsLength() > 0);

		delete pbits;
	}

	void StreamWithAttachedBitmap_DeleteStream_DestroysBitmap()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BBitmap* pbits = new BBitmap(BRect(0, 0, 5, 5), B_RGB32);
		BBitmapStream* pfullstream = new BBitmapStream(pbits);
		CPPUNIT_ASSERT(pfullstream->Position() == 0);
		CPPUNIT_ASSERT(pfullstream->Size() == pbits->BitsLength() + sizeof(TranslatorBitmap));

		delete pfullstream;
		pfullstream = NULL;
		// CPPUNIT_ASSERT(pbits->BitsLength() == 0);
	}

	void Header_ReadWriteByteByByte_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
		char chheader[sizeof(TranslatorBitmap)], *pch;
		TranslatorBitmap sheader;
		BBitmapStream stream;
		int32 width = 5, height = 5;

		sheader.magic = B_TRANSLATOR_BITMAP;
		sheader.bounds.left = 0;
		sheader.bounds.top = 0;
		sheader.bounds.right = width - 1;
		sheader.bounds.bottom = height - 1;
		sheader.rowBytes = width * 4;
		sheader.colors = B_RGB32;
		sheader.dataSize = sheader.rowBytes * height;

		memcpy(&chheader, &sheader, sizeof(TranslatorBitmap));
		CPPUNIT_ASSERT_EQUAL(B_OK, swap_data(B_UINT32_TYPE, &(chheader[0]), sizeof(TranslatorBitmap),
			B_SWAP_HOST_TO_BENDIAN));

		off_t nPos;
		for (nPos = 0; nPos < sizeof(TranslatorBitmap); nPos++) {
			pch = (char*)(&(chheader[0])) + nPos;
			CPPUNIT_ASSERT(stream.WriteAt(nPos, pch, 1) == 1);
		}

		CPPUNIT_ASSERT(stream.Size() == sizeof(TranslatorBitmap) + sheader.dataSize);

		char chbuf[sizeof(TranslatorBitmap)];
		for (nPos = 0; nPos < sizeof(TranslatorBitmap); nPos++) {
			pch = (char*)(&(chheader[0])) + nPos;
			CPPUNIT_ASSERT(stream.ReadAt(nPos, &(chbuf[0]), 1) == 1);
			CPPUNIT_ASSERT(pch[0] == chbuf[0]);
		}
	}

	void Header_ReadWriteAllAtOnce_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
		char chheader[sizeof(TranslatorBitmap)], *pch;
		TranslatorBitmap sheader;
		BBitmapStream stream;
		int32 width = 5, height = 5;

		sheader.magic = B_TRANSLATOR_BITMAP;
		sheader.bounds.left = 0;
		sheader.bounds.top = 0;
		sheader.bounds.right = width - 1;
		sheader.bounds.bottom = height - 1;
		sheader.rowBytes = width * 4;
		sheader.colors = B_RGB32;
		sheader.dataSize = sheader.rowBytes * height;

		memcpy(&chheader, &sheader, sizeof(TranslatorBitmap));
		CPPUNIT_ASSERT_EQUAL(B_OK, swap_data(B_UINT32_TYPE, &(chheader[0]), sizeof(TranslatorBitmap),
								 B_SWAP_HOST_TO_BENDIAN));

		pch = (char*)(&(chheader[0]));
		CPPUNIT_ASSERT(stream.WriteAt(0, pch, sizeof(TranslatorBitmap))
					   == sizeof(TranslatorBitmap));

		CPPUNIT_ASSERT(stream.Size() == sizeof(TranslatorBitmap) + sheader.dataSize);

		char chbuf[sizeof(TranslatorBitmap)];
		CPPUNIT_ASSERT(stream.ReadAt(0, &(chbuf[0]), sizeof(TranslatorBitmap))
					   == sizeof(TranslatorBitmap));
		CPPUNIT_ASSERT(memcmp(pch, &(chbuf[0]), sizeof(TranslatorBitmap)) == 0);
	}

	void BitmapData_ReadWrite_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
		TranslatorBitmap sheader;
		BBitmapStream stream;
		int32 width = 5, height = 5;

		sheader.magic = B_TRANSLATOR_BITMAP;
		sheader.bounds.left = 0;
		sheader.bounds.top = 0;
		sheader.bounds.right = width - 1;
		sheader.bounds.bottom = height - 1;
		sheader.rowBytes = width * 4;
		sheader.colors = B_RGB32;
		sheader.dataSize = sheader.rowBytes * height;

		char chheader[sizeof(TranslatorBitmap)];
		memcpy(&chheader, &sheader, sizeof(TranslatorBitmap));
		CPPUNIT_ASSERT_EQUAL(B_OK, swap_data(B_UINT32_TYPE, &(chheader[0]), sizeof(TranslatorBitmap),
								 B_SWAP_HOST_TO_BENDIAN));

		CPPUNIT_ASSERT(stream.WriteAt(0, chheader, sizeof(TranslatorBitmap))
					   == sizeof(TranslatorBitmap));

		int32 bytesLeft = sheader.dataSize;
		char byt = 0xCF;
		off_t nPos = sizeof(TranslatorBitmap);
		while (bytesLeft--)
			CPPUNIT_ASSERT(stream.WriteAt(nPos++, &byt, 1) == 1);

		CPPUNIT_ASSERT(stream.Size() == sizeof(TranslatorBitmap) + sheader.dataSize);

#if !TEST_R5
		// This test fails with Be's version because of a bug.
		// Be's BBitmapStream::ReadAt() has strange behavior in cases
		// where the pos parameter of ReadAt() is != BBitmapStream::Position().
		// If BBitmapStream::Read() is used instead, it calls 
		// BBitmapStream::ReadAt() with pos = BBitmapStream::Position(),
		// so, this issue is rarely a problem because Read() is most often used.
		bytesLeft = sheader.dataSize;
		nPos = sizeof(TranslatorBitmap);
		while (bytesLeft--) {
			uint8 chbuf[1] = {0x99};

			ssize_t rd = stream.ReadAt(nPos++, &(chbuf[0]), 1);
			CPPUNIT_ASSERT(rd == 1);
			CPPUNIT_ASSERT(chbuf[0] == byt);
		}
#endif
	}

	void ZeroBytes_ReadWrite_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
		BBitmapStream stream;
		char chbuf[1];
		// CPPUNIT_ASSERT(stream.ReadAt(stream.Size(), &(chbuf[0]), 0) == 0);
		CPPUNIT_ASSERT(stream.ReadAt(0 + 1000, &(chbuf[0]), 0) == 0);
		CPPUNIT_ASSERT(stream.ReadAt(-1, &(chbuf[0]), 0) == 0);
	}

	void ErroneousData_ReadWrite_ReturnsErrors()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");
		BBitmapStream stream;
		char byt = 0xCF;
		char chbuf[1];

		CPPUNIT_ASSERT(stream.WriteAt(0, &byt, 0) == 0);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.WriteAt(-1, &byt, 1));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.WriteAt(stream.Size(), &byt, 1));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.WriteAt(stream.Size() + 1, &byt, 1));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.WriteAt(0, NULL, 1));

		CPPUNIT_ASSERT(stream.ReadAt(0, &(chbuf[0]), 0) == 0);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.ReadAt(-1, &(chbuf[0]), 1));
		// CPPUNIT_ASSERT_EQUAL(B_ERROR, stream.ReadAt(stream.Size(), &(chbuf[0]), 1));
		// CPPUNIT_ASSERT_EQUAL(B_ERROR, stream.ReadAt(stream.Size() + 1, &(chbuf[0]), 1));
#if !TEST_R5
		// Be's version doesn't check for NULL
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.ReadAt(0, NULL, 1));
#endif
	}

	void SEEK_END_Seek_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BFile file("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());

		BBitmapStream stream;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP));

		off_t nPos;
		nPos = stream.Size();
		CPPUNIT_ASSERT(stream.Seek(0, SEEK_END) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		nPos = 0;
		CPPUNIT_ASSERT(stream.Seek(-stream.Size(), SEEK_END) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		nPos = stream.Size() - 15;
		CPPUNIT_ASSERT(stream.Seek(-15, SEEK_END) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.Seek(1, SEEK_END));
		CPPUNIT_ASSERT(stream.Position() == nPos);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.Seek(-(stream.Size() + 1), SEEK_END));
		CPPUNIT_ASSERT(stream.Position() == nPos);
	}

	void SEEK_SET_Seek_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BFile file("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());

		BBitmapStream stream;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP));

		off_t nPos = 0;
		CPPUNIT_ASSERT(stream.Seek(0, SEEK_SET) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		nPos = stream.Size();
		CPPUNIT_ASSERT(stream.Seek(nPos, SEEK_SET) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		nPos /= 2;
		CPPUNIT_ASSERT(stream.Seek(nPos, SEEK_SET) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.Seek(-1, SEEK_SET));
		CPPUNIT_ASSERT(stream.Position() == nPos);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.Seek(stream.Size() + 1, SEEK_SET));
		CPPUNIT_ASSERT(stream.Position() == nPos);
	}

	void SEEK_CUR_Seek_Succeeds()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BFile file("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());

		BBitmapStream stream;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP));

		off_t nPos = stream.Size() / 2;
		stream.Seek(nPos, SEEK_SET);

		CPPUNIT_ASSERT(stream.Seek(-nPos, SEEK_CUR) == 0);
		CPPUNIT_ASSERT(stream.Position() == 0);

		nPos = stream.Size();
		CPPUNIT_ASSERT(stream.Seek(nPos, SEEK_CUR) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		nPos -= 11;
		CPPUNIT_ASSERT(stream.Seek(-11, SEEK_CUR) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		nPos += 8;
		CPPUNIT_ASSERT(stream.Seek(8, SEEK_CUR) == nPos);
		CPPUNIT_ASSERT(stream.Position() == nPos);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.Seek(-(stream.Position() + 1), SEEK_CUR));
		CPPUNIT_ASSERT(stream.Position() == nPos);

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.Seek((stream.Size() - stream.Position()) + 1, SEEK_CUR));
		CPPUNIT_ASSERT(stream.Position() == nPos);
	}

	void InvalidValues_SetSize_ReturnsBadValue()
	{
		BApplication app("application/x-vnd.Haiku-translationkit_bitmapstreamtest");

		BFile file("resources/kits/translation/image.jpg", B_READ_ONLY);
		CPPUNIT_ASSERT_EQUAL(B_OK, file.InitCheck());

		BBitmapStream stream;
		BTranslatorRoster* proster = BTranslatorRoster::Default();
		CPPUNIT_ASSERT_EQUAL(B_OK, proster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP));

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.SetSize(-1));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, stream.SetSize(stream.Size() + 1));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(BitmapStreamTest, getTestSuiteName());
