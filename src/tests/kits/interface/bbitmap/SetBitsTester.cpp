//------------------------------------------------------------------------------
//	SetBitsTester.cpp
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
#include "SetBitsTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

struct set_bits_test_data {
	color_space	space;
	int32 		width;
	int32 		height;
	int32		offset;
	int32		length;
	int32		pixel_count;
	uint8		data[256];
};

// B_RGB32, width = 3
set_bits_test_data rgb32_test_data3_initial = {
	B_RGB32, 3, 2,
	8, 24, 6,
	{
		  0,   0,   0, 255,		  8,   8,   8, 255,		128, 128, 128, 255,
		255, 255, 255, 255,		  0,   0, 228, 255,		  0, 102,   0, 255,
	}
};

set_bits_test_data rgb32_test_data3a_initial = {
	B_RGB32, 3, 2,
	8, 24, 6,
	{
		  0,   0,   0,   0,		  8,   8,   8,   0,		128, 128, 128,   0,
		255, 255, 255,   0,		  0,   0, 228,   0,		  0, 102,   0,   0,
	}
};

set_bits_test_data rgb32_test_data3_set = {
	B_RGB32, 3, 2,
	0, 18, 6,
	{
		255, 255, 152,		 51, 102, 102,		 51, 255, 203,
		  0, 102,  51,		203, 152, 203,		203, 203,   0,
	}
};

set_bits_test_data rgb32_test_data3p_set = {
	B_RGB32, 3, 1,
	0, 9, 3,
	{
		 51, 255, 203,		0, 102,  51,		203, 152, 203,
	}
};

set_bits_test_data rgb32_test_data3_final = {
	B_RGB32, 3, 2,
	0, 24, 6,
	{
		152, 255, 255, 255,		102, 102,  51, 255,		203, 255,  51, 255,
		 51, 102,   0, 255,		203, 152, 203, 255,		  0, 203, 203, 255,
	}
};

set_bits_test_data rgb32_test_data3a_final = {
	B_RGB32, 3, 2,
	0, 24, 6,
	{
		152, 255, 255,   0,		102, 102,  51,   0,		203, 255,  51,   0,
		 51, 102,   0,   0,		203, 152, 203,   0,		  0, 203, 203,   0,
	}
};

set_bits_test_data rgb32_test_data3g_final = {
	B_RGB32, 3, 2,
	0, 24, 6,
	{
		255, 255, 255, 255,		  0,   0,   0, 255,		255, 255, 255, 255,
		  0,   0,   0, 255,		255, 255, 255, 255,		255, 255, 255, 255,
	}
};

set_bits_test_data rgb32_test_data3p_final = {
	B_RGB32, 3, 2,
	0, 24, 6,
	{
		  0,   0,   0, 255,		  8,   8,   8, 255,		203, 255,  51, 255,
		 51, 102,   0, 255,		203, 152, 203, 255,		  0, 102,   0, 255,
	}
};

set_bits_test_data rgb32_test_data3ap_final = {
	B_RGB32, 3, 2,
	0, 24, 6,
	{
		  0,   0,   0,   0,		  8,   8,   8,   0,		203, 255,  51,   0,
		 51, 102,   0,   0,		203, 152, 203,   0,		  0, 102,   0,   0,
	}
};

// B_RGB32, width = 4
set_bits_test_data rgb32_test_data4_initial = {
	B_RGB32, 4, 2,
	8, 32, 8,
	{
		  0,   0,   0, 255,		  8,   8,   8, 255,		128, 128, 128, 255,
		255, 255, 255, 255,		  0,   0, 228, 255,		  0, 102,   0, 255,
		255,   0,   0, 255,		152, 102,   0, 255,
	}
};

set_bits_test_data rgb32_test_data4a_initial = {
	B_RGB32, 4, 2,
	8, 32, 8,
	{
		  0,   0,   0,   0,		  8,   8,   8,   0,		128, 128, 128,   0,
		255, 255, 255,   0,		  0,   0, 228,   0,		  0, 102,   0,   0,
		255,   0,   0,   0,		152, 102,   0,   0,
	}
};

set_bits_test_data rgb32_test_data4_set = {
	B_RGB32, 4, 2,
	0, 24, 8,
	{
		255, 255, 152,		 51, 102, 102,		 51, 255, 203,
		  0, 102,  51,		203, 152, 203,		203, 203,   0,
		152, 255, 203,		  0,  30,   0,
	}
};

set_bits_test_data rgb32_test_data4p_set = rgb32_test_data3p_set;

set_bits_test_data rgb32_test_data4_final = {
	B_RGB32, 4, 2,
	0, 32, 8,
	{
		152, 255, 255, 255,		102, 102,  51, 255,		203, 255,  51, 255,
		 51, 102,   0, 255,		203, 152, 203, 255,		  0, 203, 203, 255,
		203, 255, 152, 255,		  0,  30,   0, 255,
	}
};

set_bits_test_data rgb32_test_data4a_final = {
	B_RGB32, 4, 2,
	0, 32, 8,
	{
		152, 255, 255,   0,		102, 102,  51,   0,		203, 255,  51,   0,
		 51, 102,   0,   0,		203, 152, 203,   0,		  0, 203, 203,   0,
		203, 255, 152,   0,		  0,  30,   0,   0,
	}
};

set_bits_test_data rgb32_test_data4g_final = {
	B_RGB32, 4, 2,
	0, 32, 8,
	{
		255, 255, 255, 255,		  0,   0,   0, 255,		255, 255, 255, 255,
		  0,   0,   0, 255,		255, 255, 255, 255,		255, 255, 255, 255,
		255, 255, 255, 255,		  0,   0,   0, 255,
	}
};

set_bits_test_data rgb32_test_data4p_final = {
	B_RGB32, 4, 2,
	0, 32, 8,
	{
		  0,   0,   0, 255,		  8,   8,   8, 255,		203, 255,  51, 255,
		 51, 102,   0, 255,		203, 152, 203, 255,		  0, 102,   0, 255,
		255,   0,   0, 255,		152, 102,   0, 255,
	}
};

set_bits_test_data rgb32_test_data4ap_final = {
	B_RGB32, 4, 2,
	0, 32, 8,
	{
		  0,   0,   0,   0,		  8,   8,   8,   0,		203, 255,  51,   0,
		 51, 102,   0,   0,		203, 152, 203,   0,		  0, 102,   0,   0,
		255,   0,   0,   0,		152, 102,   0,   0,
	}
};

// B_CMAP8, width = 3
set_bits_test_data cmap8_test_data3_initial = {
	B_CMAP8, 3, 2,
	2, 8, 6,
	{
		  0,   1,  16,   0, 255,  43, 159,   0,
	}
};

set_bits_test_data cmap8_test_data3_set = {
	B_CMAP8, 3, 2,
	0, 8, 6,
	{
		253, 181,  83,   0, 158, 129, 101,   0,
	}
};

set_bits_test_data cmap8_test_data3a_set = {
	B_CMAP8, 3, 2,
	0, 6, 6,
	{
		253, 181,  83, 158, 129, 101,
	}
};

set_bits_test_data cmap8_test_data3p_set = {
	B_CMAP8, 3, 1,
	0, 4, 3,
	{
		 83,   0, 158, 129,
	}
};

set_bits_test_data cmap8_test_data3ap_set = {
	B_CMAP8, 3, 1,
	0, 3, 3,
	{
		 83, 158, 129,
	}
};

set_bits_test_data &cmap8_test_data3_final = cmap8_test_data3_set;

set_bits_test_data cmap8_test_data3g_final = {
	B_CMAP8, 3, 2,
	0, 8, 6,
	{
		 63,   0,  63,   0,   0,  63,  63,   0,
	}
};

set_bits_test_data cmap8_test_data3p_final = {
	B_CMAP8, 3, 2,
	0, 8, 6,
	{
		  0,   1,  83,   0, 158, 129, 159,   0,
	}
};

// B_CMAP8, width = 4
set_bits_test_data cmap8_test_data4_initial = {
	B_CMAP8, 4, 2,
	2, 8, 8,
	{
		  0,   1,  16, 255,  43, 159,  32, 126
	}
};

set_bits_test_data cmap8_test_data4_set = {
	B_CMAP8, 4, 2,
	0, 8, 8,
	{
		253, 181,  83, 158, 129, 101,  71,  61
	}
};

set_bits_test_data cmap8_test_data4p_set = {
	B_CMAP8, 4, 2,
	0, 3, 3,
	{
		 83, 158, 129,
	}
};

set_bits_test_data &cmap8_test_data4_final = cmap8_test_data4_set;

set_bits_test_data cmap8_test_data4g_final = {
	B_CMAP8, 4, 2,
	0, 8, 8,
	{
		 63,   0,  63,   0,  63,  63,  63,   0
	}
};

set_bits_test_data cmap8_test_data4p_final = {
	B_CMAP8, 4, 2,
	2, 8, 8,
	{
		  0,   1,  83, 158, 129, 159,  32, 126
	}
};

// B_RGB32_BIG
set_bits_test_data rgb32_big_test_data4_initial = {
	B_RGB32_BIG, 4, 2,
	0, 32, 8,
	{
		  0,   0,   0, 255,		  8,   8,   8, 255,		128, 128, 128, 255,
		255, 255, 255, 255,		228,   0,   0, 255,		  0, 102,   0, 255,
		  0,   0, 255, 255,		  0, 102, 152, 255,
	}
};

set_bits_test_data rgb32_big_test_data4_set = {
	B_RGB32_BIG, 4, 2,
	0, 32, 8,
	{
		255, 255, 152, 255,		 51, 102, 102, 255,		 51, 255, 203, 255,
		  0, 102,  51, 255,		203, 152, 203, 255,		203, 203,   0, 255,
		152, 255, 203, 255,		  0,  30,   0, 255,
	}
};

set_bits_test_data rgb32_big_test_data4_final = {
	B_RGB32_BIG, 4, 2,
	0, 32, 8,
	{
		255, 255, 152,   0,		 51, 102, 102,   0,		 51, 255, 203,   0,
		  0, 102,  51,   0,		203, 152, 203,   0,		203, 203,   0,   0,
		152, 255, 203,   0,		  0,  30,   0,   0,
	}
};

// B_GRAY1, width = 3
set_bits_test_data gray1_test_data3_initial = {
	B_GRAY1, 3, 2,
	0, 8, 6,
	{
		0x20, 0x00, 0x00, 0x00,	// 001
		0x80, 0x00, 0x00, 0x00,	// 100
	}
};

set_bits_test_data gray1_test_data3_set = {
	B_GRAY1, 3, 2,
	0, 8, 6,
	{
		0xa0, 0x00, 0x00, 0x00,	// 101
		0x60, 0x00, 0x00, 0x00,	// 011
	}
};

set_bits_test_data &gray1_test_data3_final = gray1_test_data3_set;

// B_GRAY1, width = 4
set_bits_test_data gray1_test_data4_initial = {
	B_GRAY1, 4, 2,
	0, 8, 8,
	{
		0x30, 0x00, 0x00, 0x00,	// 0011
		0x00, 0x00, 0x00, 0x00,	// 0000
	}
};

set_bits_test_data gray1_test_data4_set = {
	B_GRAY1, 4, 2,
	0, 8, 8,
	{
		0xa0, 0x00, 0x00, 0x00,	// 1010
		0xe0, 0x00, 0x00, 0x00,	// 1110
	}
};

set_bits_test_data &gray1_test_data4_final = gray1_test_data4_set;

#if 0
// dump_bitmap
static
void
dump_bitmap(const BBitmap *bitmap)
{
	BRect bounds = bitmap->Bounds();
	int32 width = bounds.IntegerWidth() + 1;
	int32 height = bounds.IntegerHeight() + 1;
	printf("BBitmap: %ldx%ld, %x\n", width, height, bitmap->ColorSpace());
	int32 bpr = bitmap->BytesPerRow();
	const uint8 *bits = (const uint8*)bitmap->Bits();
	for (int32 y = 0; y < height; y++) {
		const uint8 *row = bits + y * bpr;
		for (int32 i = 0; i < bpr; i++)
			printf("%02x ", row[i]);
		printf("\n");
	}
}
#endif

// test_set_bits
static
void
test_set_bits(const set_bits_test_data &initialData,
			  const set_bits_test_data &setData,
			  const set_bits_test_data &finalData)
{
	BRect bounds(0, 0, initialData.width - 1, initialData.height - 1);
	BBitmap bitmap(bounds, initialData.space);
	CHK(bitmap.InitCheck() == B_OK);
	CHK(bitmap.Bounds() == bounds);
	CHK(bitmap.BitsLength() == initialData.length);
	memcpy(bitmap.Bits(), initialData.data, initialData.length);
//dump_bitmap(&bitmap);
//printf("bitmap.SetBits(%p, %ld, %ld, %x)\n", setData.data, setData.length, 0L,
//setData.space);
	bitmap.SetBits(setData.data, setData.length, 0, setData.space);
//dump_bitmap(&bitmap);
	CHK(memcmp(bitmap.Bits(), finalData.data, finalData.length) == 0);
}

// test_set_bits2
static
void
test_set_bits2(const set_bits_test_data &initialData,
			   const set_bits_test_data &setData,
			   const set_bits_test_data &finalData)
{
	BRect bounds(0, 0, initialData.width - 1, initialData.height - 1);
	BBitmap bitmap(bounds, initialData.space);
	CHK(bitmap.InitCheck() == B_OK);
	CHK(bitmap.Bounds() == bounds);
	CHK(bitmap.BitsLength() == initialData.length);
	memcpy(bitmap.Bits(), initialData.data, initialData.length);
//dump_bitmap(&bitmap);
//printf("bitmap.SetBits(%p, %ld, %ld, %x)\n", setData.data, setData.length,
//initialData.offset, setData.space);
	bitmap.SetBits(setData.data, setData.length, initialData.offset,
				   setData.space);
//dump_bitmap(&bitmap);
	CHK(memcmp(bitmap.Bits(), finalData.data, finalData.length) == 0);
}

// test for Haiku only API
#ifndef TEST_R5

// test_import_bits
static
void
test_import_bits(const set_bits_test_data &initialData,
				 const set_bits_test_data &setData,
				 const set_bits_test_data &finalData)
{
	// init bitmap 1
	BRect bounds(0, 0, initialData.width - 1, initialData.height - 1);
	BBitmap bitmap(bounds, initialData.space);
	CHK(bitmap.InitCheck() == B_OK);
	CHK(bitmap.Bounds() == bounds);
	CHK(bitmap.BitsLength() == initialData.length);
	memcpy(bitmap.Bits(), initialData.data, initialData.length);
	// init bitmap 2
	BRect bounds2(0, 0, setData.width - 1, setData.height - 1);
	BBitmap bitmap2(bounds2, setData.space);
	CHK(bitmap2.InitCheck() == B_OK);
	CHK(bitmap2.Bounds() == bounds2);
	CHK(bitmap2.BitsLength() == setData.length);
	memcpy(bitmap2.Bits(), setData.data, setData.length);
	// import bits
	CHK(bounds == bounds2);
	CHK(initialData.length == finalData.length);
//dump_bitmap(&bitmap);
//printf("bitmap.ImportBits(): (%ld, %ld, %x) -> (%ld, %ld, %x)\n",
//bounds.IntegerWidth() + 1, bounds.IntegerHeight() + 1, initialData.space,
//bounds2.IntegerWidth() + 1, bounds2.IntegerHeight() + 1, setData.space);
	CHK(bitmap.ImportBits(&bitmap2) == B_OK);
//dump_bitmap(&bitmap);
	CHK(memcmp(bitmap.Bits(), finalData.data, finalData.length) == 0);
}

#endif	// test for Haiku only API

/*
	void SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace)
	@case 1			overwrite complete bitmap data, offset = 0,
					different color spaces, no row padding
	@results		Bitmap should contain the correct data.
 */
void SetBitsTester::SetBits1()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
#ifdef TEST_R5
	test_set_bits(rgb32_test_data3_initial, rgb32_test_data3_set,
				  rgb32_test_data3a_final);
	test_set_bits(rgb32_test_data4_initial, rgb32_test_data4_set,
				  rgb32_test_data4a_final);
	test_set_bits(cmap8_test_data4_initial, cmap8_test_data4_set,
				  cmap8_test_data4_final);

	test_set_bits(rgb32_test_data4a_initial, cmap8_test_data4_set,
				  rgb32_test_data4_final);
	test_set_bits(cmap8_test_data4_initial, rgb32_test_data4_set,
				  cmap8_test_data4_final);
#else
	test_set_bits(rgb32_test_data3_initial, rgb32_test_data3_set,
				  rgb32_test_data3_final);
	test_set_bits(rgb32_test_data4_initial, rgb32_test_data4_set,
				  rgb32_test_data4_final);
	test_set_bits(cmap8_test_data4_initial, cmap8_test_data4_set,
				  cmap8_test_data4_final);

	test_set_bits(rgb32_test_data4_initial, cmap8_test_data4_set,
				  rgb32_test_data4_final);
	test_set_bits(cmap8_test_data4_initial, rgb32_test_data4_set,
				  cmap8_test_data4_final);
#endif
}

/*
	void SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace)
	@case 2			overwrite complete bitmap data, offset = 0,
					different color spaces, row padding
	@results		Bitmap should contain the correct data.
 */
void SetBitsTester::SetBits2()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
	test_set_bits(cmap8_test_data3_initial, cmap8_test_data3_set,
				  cmap8_test_data3_final);
	test_set_bits(gray1_test_data3_initial, gray1_test_data3_set,
				  gray1_test_data3_final);
	test_set_bits(gray1_test_data4_initial, gray1_test_data4_set,
				  gray1_test_data4_final);

// ignores source row padding
	test_set_bits(rgb32_test_data3a_initial, cmap8_test_data3a_set,
				  rgb32_test_data3_final);
#ifndef TEST_R5
// R5: broken: no effect
	test_set_bits(rgb32_test_data3a_initial, gray1_test_data3_set,
				  rgb32_test_data3g_final);
	test_set_bits(rgb32_test_data4a_initial, gray1_test_data4_set,
				  rgb32_test_data4g_final);
// R5: broken: ignores target bitmap row padding
	test_set_bits(cmap8_test_data3_initial, rgb32_test_data3_set,
				  cmap8_test_data3_final);
// R5: broken: simply copies the data
	test_set_bits(cmap8_test_data3_initial, gray1_test_data3_set,
				  cmap8_test_data3g_final);
	test_set_bits(cmap8_test_data4_initial, gray1_test_data4_set,
				  cmap8_test_data4g_final);
#endif
}

/*
	void SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace)
	@case 3			overwrite bitmap data partially, offset = 0,
					different color spaces, no row padding
	@results		Bitmap should contain the correct data.
 */
void SetBitsTester::SetBits3()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
#ifdef TEST_R5
	test_set_bits2(rgb32_test_data3a_initial, rgb32_test_data3p_set,
				   rgb32_test_data3ap_final);
	test_set_bits2(rgb32_test_data4a_initial, rgb32_test_data4p_set,
				   rgb32_test_data4ap_final);
	test_set_bits2(cmap8_test_data4_initial, cmap8_test_data4p_set,
				   cmap8_test_data4p_final);
#else
	test_set_bits2(rgb32_test_data3_initial, rgb32_test_data3p_set,
				   rgb32_test_data3p_final);
	test_set_bits2(rgb32_test_data4_initial, rgb32_test_data4p_set,
				   rgb32_test_data4p_final);
	test_set_bits2(cmap8_test_data4_initial, cmap8_test_data4p_set,
				   cmap8_test_data4p_final);
#endif

	test_set_bits2(rgb32_test_data4_initial, cmap8_test_data4p_set,
				   rgb32_test_data4p_final);
	test_set_bits2(cmap8_test_data4_initial, rgb32_test_data4p_set,
				   cmap8_test_data4p_final);
}

/*
	void SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace)
	@case 4			overwrite bitmap data partially, offset = 0,
					different color spaces, row padding
	@results		Bitmap should contain the correct data.
 */
void SetBitsTester::SetBits4()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
	test_set_bits2(cmap8_test_data3_initial, cmap8_test_data3p_set,
				   cmap8_test_data3p_final);

	test_set_bits2(rgb32_test_data3_initial, cmap8_test_data3ap_set,
				   rgb32_test_data3p_final);
// R5: broken: ignores target bitmap row padding
#ifndef TEST_R5
	test_set_bits2(cmap8_test_data3_initial, rgb32_test_data3p_set,
				   cmap8_test_data3p_final);
#endif
}

// Haiku only API
#ifndef TEST_R5

/*
	status_t ImportBits(const BBitmap *bitmap)
	@case 1			NULL bitmap
	@results		Should return B_BAD_VALUE.
 */
void SetBitsTester::ImportBitsA1()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
	BBitmap bitmap(BRect(0, 0, 9, 9), B_RGB32);
	CHK(bitmap.ImportBits(NULL) == B_BAD_VALUE);
}

/*
	status_t ImportBits(const BBitmap *bitmap)
	@case 2			bitmap with different Bounds()
	@results		Should return B_BAD_VALUE.
 */
void SetBitsTester::ImportBitsA2()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
	BBitmap bitmap(BRect(0, 0, 9, 9), B_RGB32);
	BBitmap bitmap2(BRect(0, 0, 9, 10), B_RGB32);
	CHK(bitmap.ImportBits(&bitmap2) == B_BAD_VALUE);
}

/*
	status_t ImportBits(const BBitmap *bitmap)
	@case 3			this and bitmap are properly initialized and have the
					same bounds, different color spaces
	@results		Should set the data and return B_OK.
 */
void SetBitsTester::ImportBitsA3()
{
	BApplication app("application/x-vnd.obos.bitmap-setbits-test");
	// B_RGB32
	test_import_bits(rgb32_test_data3_initial, rgb32_test_data3_final,
					 rgb32_test_data3_final);
	test_import_bits(rgb32_test_data3_initial, cmap8_test_data3_final,
					 rgb32_test_data3_final);
	test_import_bits(rgb32_test_data3_initial, gray1_test_data3_final,
					 rgb32_test_data3g_final);
	test_import_bits(rgb32_test_data4_initial, rgb32_test_data4_final,
					 rgb32_test_data4_final);
	test_import_bits(rgb32_test_data4_initial, cmap8_test_data4_final,
					 rgb32_test_data4_final);
	test_import_bits(rgb32_test_data4_initial, gray1_test_data4_final,
					 rgb32_test_data4g_final);
	// B_CMAP8
	test_import_bits(cmap8_test_data3_initial, rgb32_test_data3_final,
					 cmap8_test_data3_final);
	test_import_bits(cmap8_test_data3_initial, cmap8_test_data3_final,
					 cmap8_test_data3_final);
	test_import_bits(cmap8_test_data3_initial, gray1_test_data3_final,
					 cmap8_test_data3g_final);
	test_import_bits(cmap8_test_data4_initial, rgb32_test_data4_final,
					 cmap8_test_data4_final);
	test_import_bits(cmap8_test_data4_initial, cmap8_test_data4_final,
					 cmap8_test_data4_final);
	test_import_bits(cmap8_test_data4_initial, gray1_test_data4_final,
					 cmap8_test_data4g_final);
	// B_GRAY1
	test_import_bits(gray1_test_data3_initial, rgb32_test_data3_final,
					 gray1_test_data3_final);
	test_import_bits(gray1_test_data3_initial, cmap8_test_data3_final,
					 gray1_test_data3_final);
	test_import_bits(gray1_test_data3_initial, gray1_test_data3_final,
					 gray1_test_data3_final);
	test_import_bits(gray1_test_data4_initial, rgb32_test_data4_final,
					 gray1_test_data4_final);
	test_import_bits(gray1_test_data4_initial, cmap8_test_data4_final,
					 gray1_test_data4_final);
	test_import_bits(gray1_test_data4_initial, gray1_test_data4_final,
					 gray1_test_data4_final);
}
#endif // ifndef TEST_R5

Test* SetBitsTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, SetBits1);
	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, SetBits2);
	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, SetBits3);
	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, SetBits4);

// Haiku only API
#ifndef TEST_R5
	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, ImportBitsA1);
	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, ImportBitsA2);
	ADD_TEST4(BBitmap, SuiteOfTests, SetBitsTester, ImportBitsA3);
#endif

	return SuiteOfTests;
}

