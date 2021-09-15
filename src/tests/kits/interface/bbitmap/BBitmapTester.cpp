//------------------------------------------------------------------------------
//	BBitmapTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Application.h>
#include <Bitmap.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "BBitmapTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// get_bytes_per_row
static inline
int32
get_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = 0;
	switch (colorSpace) {
		// supported
		case B_RGBA64: case B_RGBA64_BIG:
			bpr = 8 * width;
			break;
		case B_RGB48: case B_RGB48_BIG:
			bpr = 6 * width;
			break;
		case B_RGB32: case B_RGBA32:
		case B_RGB32_BIG: case B_RGBA32_BIG:
		case B_UVL32: case B_UVLA32:
		case B_LAB32: case B_LABA32:
		case B_HSI32: case B_HSIA32:
		case B_HSV32: case B_HSVA32:
		case B_HLS32: case B_HLSA32:
		case B_CMY32: case B_CMYA32: case B_CMYK32:
			bpr = 4 * width;
			break;
		case B_RGB24: case B_RGB24_BIG:
		case B_UVL24: case B_LAB24: case B_HSI24:
		case B_HSV24: case B_HLS24: case B_CMY24:
			bpr = 3 * width;
			break;
		case B_RGB16:		case B_RGB15:		case B_RGBA15:
		case B_RGB16_BIG:	case B_RGB15_BIG:	case B_RGBA15_BIG:
			bpr = 2 * width;
			break;
		case B_CMAP8: case B_GRAY8:
			bpr = width;
			break;
		case B_GRAY1:
			bpr = (width + 7) / 8;
			break;
		case B_YCbCr422: case B_YUV422:
			bpr = (width + 3) / 4 * 8;
			break;
		case B_YCbCr411: case B_YUV411:
			bpr = (width + 3) / 4 * 6;
			break;
		case B_YCbCr444: case B_YUV444:
			bpr = (width + 3) / 4 * 12;
			break;
		case B_YCbCr420: case B_YUV420:
			bpr = (width + 3) / 4 * 6;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV9: case B_YUV12:
			break;
	}
	// align to int32
	bpr = (bpr + 3) & 0x7ffffffc;
	return bpr;
}


/*
	BBitmap(BRect bounds, color_space colorSpace, bool acceptsViews,
			bool needsContiguous);
	@case 1			acceptsViews = false, needsContiguous = false, valid bounds,
					different color spaces
	@results		Bits() should be valid, BitsLength(), BytesPerRow() etc.
					should return the correct values.
 */
void TBBitmapTester::BBitmap1()
{
	BApplication app("application/x-vnd.obos.bitmap-constructor-test");
	struct test_case {
		BRect		bounds;
		color_space	space;
	} testCases[] = {
		{ BRect(0, 0, 39, 9),	B_RGBA64 },
		{ BRect(0, 0, 39, 9),	B_RGBA64_BIG },
		{ BRect(0, 0, 39, 9),	B_RGB48 },
		{ BRect(0, 0, 39, 9),	B_RGB48_BIG },

		{ BRect(0, 0, 39, 9),	B_RGB32 },
		{ BRect(0, 0, 39, 9),	B_RGBA32 },
		{ BRect(0, 0, 39, 9),	B_RGB32_BIG },
		{ BRect(0, 0, 39, 9),	B_RGBA32_BIG },

		{ BRect(0, 0, 39, 9),	B_UVL32 },
		{ BRect(0, 0, 39, 9),	B_UVLA32 },
		{ BRect(0, 0, 39, 9),	B_LAB32 },
		{ BRect(0, 0, 39, 9),	B_LABA32 },
		{ BRect(0, 0, 39, 9),	B_HSI32 },
		{ BRect(0, 0, 39, 9),	B_HSIA32 },
		{ BRect(0, 0, 39, 9),	B_HSV32 },
		{ BRect(0, 0, 39, 9),	B_HSVA32 },
		{ BRect(0, 0, 39, 9),	B_HLS32 },
		{ BRect(0, 0, 39, 9),	B_HLSA32 },
		{ BRect(0, 0, 39, 9),	B_CMY32 },
		{ BRect(0, 0, 39, 9),	B_CMYA32 },
		{ BRect(0, 0, 39, 9),	B_CMYK32 },
		{ BRect(0, 0, 39, 9),	B_RGB24 },
		{ BRect(0, 0, 18, 9),	B_RGB24 },
		{ BRect(0, 0, 39, 9),	B_RGB24_BIG },
		{ BRect(0, 0, 18, 9),	B_RGB24_BIG },
		{ BRect(0, 0, 39, 9),	B_UVL24 },
		{ BRect(0, 0, 18, 9),	B_UVL24 },
		{ BRect(0, 0, 39, 9),	B_LAB24 },
		{ BRect(0, 0, 18, 9),	B_LAB24 },
		{ BRect(0, 0, 39, 9),	B_HSI24 },
		{ BRect(0, 0, 18, 9),	B_HSI24 },
		{ BRect(0, 0, 39, 9),	B_HSV24 },
		{ BRect(0, 0, 18, 9),	B_HSV24 },
		{ BRect(0, 0, 39, 9),	B_HLS24 },
		{ BRect(0, 0, 18, 9),	B_HLS24 },
		{ BRect(0, 0, 39, 9),	B_CMY24 },
		{ BRect(0, 0, 18, 9),	B_CMY24 },
		{ BRect(0, 0, 39, 9),	B_RGB16 },
		{ BRect(0, 0, 18, 9),	B_RGB16 },
		{ BRect(0, 0, 39, 9),	B_RGB16_BIG },
		{ BRect(0, 0, 18, 9),	B_RGB16_BIG },
		{ BRect(0, 0, 39, 9),	B_RGB15 },
		{ BRect(0, 0, 18, 9),	B_RGB15 },
		{ BRect(0, 0, 39, 9),	B_RGB15_BIG },
		{ BRect(0, 0, 18, 9),	B_RGB15_BIG },
		{ BRect(0, 0, 39, 9),	B_RGBA15 },
		{ BRect(0, 0, 18, 9),	B_RGBA15 },
		{ BRect(0, 0, 39, 9),	B_RGBA15_BIG },
		{ BRect(0, 0, 18, 9),	B_RGBA15_BIG },
		{ BRect(0, 0, 39, 9),	B_CMAP8 },
		{ BRect(0, 0, 18, 9),	B_CMAP8 },
		{ BRect(0, 0, 39, 9),	B_GRAY8 },
		{ BRect(0, 0, 18, 9),	B_GRAY8 },
		{ BRect(0, 0, 39, 9),	B_GRAY1 },
		{ BRect(0, 0, 31, 9),	B_GRAY1 },
		{ BRect(0, 0, 18, 9),	B_GRAY1 },
		{ BRect(0, 0, 39, 9),	B_YCbCr422 },
		{ BRect(0, 0, 18, 9),	B_YCbCr422 },
		{ BRect(0, 0, 39, 9),	B_YUV422 },
		{ BRect(0, 0, 18, 9),	B_YUV422 },
// R5: calculates weird BPR values for B_YCbCr411, B_YUV411 and B_YUV420.
// TODO: Investigate!
#ifndef TEST_R5
		{ BRect(0, 0, 39, 9),	B_YCbCr411 },
		{ BRect(0, 0, 31, 9),	B_YCbCr411 },
		{ BRect(0, 0, 18, 9),	B_YCbCr411 },
		{ BRect(0, 0, 39, 9),	B_YUV411 },
		{ BRect(0, 0, 31, 9),	B_YUV411 },
		{ BRect(0, 0, 18, 9),	B_YUV411 },
#endif
		{ BRect(0, 0, 39, 9),	B_YCbCr444 },
		{ BRect(0, 0, 18, 9),	B_YCbCr444 },
		{ BRect(0, 0, 39, 9),	B_YUV444 },
		{ BRect(0, 0, 18, 9),	B_YUV444 },
		{ BRect(0, 0, 39, 9),	B_YCbCr420 },
		{ BRect(0, 0, 31, 9),	B_YCbCr420 },
		{ BRect(0, 0, 18, 9),	B_YCbCr420 },
#ifndef TEST_R5
		{ BRect(0, 0, 39, 9),	B_YUV420 },
		{ BRect(0, 0, 31, 9),	B_YUV420 },
		{ BRect(0, 0, 18, 9),	B_YUV420 },
#endif
	};
	int32 testCaseCount = sizeof(testCases) / sizeof(test_case);
	for (int32 i = 0; i < testCaseCount; i++) {
		// get test case
		const test_case &testCase = testCases[i];
		int32 width = testCase.bounds.IntegerWidth() + 1;
		int32 height = testCase.bounds.IntegerHeight() + 1;
		int32 bpr = get_bytes_per_row(testCase.space, width);
		// create and check bitmap
		BBitmap bitmap(testCase.bounds, testCase.space);
		CHK(bitmap.InitCheck() == B_OK);
		CHK(bitmap.IsValid() == true);
		CHK(bitmap.Bits() != NULL);
		CHK(bitmap.Bounds() == testCase.bounds);
		CHK(bitmap.ColorSpace() == testCase.space);
if (bitmap.BytesPerRow() != bpr) {
printf("space: %x: bpr: %ld (%ld)\n", testCase.space, bitmap.BytesPerRow(),
bpr);
}
		CHK(bitmap.BytesPerRow() == bpr);
		CHK(bitmap.BitsLength() == bpr * height);
	}
}


Test* TBBitmapTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BBitmap, SuiteOfTests, TBBitmapTester, BBitmap1);

	return SuiteOfTests;
}

