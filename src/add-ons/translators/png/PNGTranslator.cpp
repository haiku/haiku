/*****************************************************************************/
// PNGTranslator
// Written by Michael Wilber, OBOS Translation Kit Team
//
// PNGTranslator.cpp
//
// This BTranslator based object is for opening and writing
// PNG images.
//
//
// Copyright (c) 2003, OpenBeOS Project
// Copyright (c) 2009, Haiku, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include "PNGTranslator.h"

#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <OS.h>
#define PNG_NO_PEDANTIC_WARNINGS
#include <png.h>

#include "PNGView.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PNGTranslator"

// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		B_PNG_FORMAT,
		B_TRANSLATOR_BITMAP,
		PNG_IN_QUALITY,
		PNG_IN_CAPABILITY,
		"image/png",
		"PNG image"
	},
	{
		B_PNG_FORMAT,
		B_TRANSLATOR_BITMAP,
		PNG_IN_QUALITY,
		PNG_IN_CAPABILITY,
		"image/x-png",
		"PNG image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (PNGTranslator)"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_PNG_FORMAT,
		B_TRANSLATOR_BITMAP,
		PNG_OUT_QUALITY,
		PNG_OUT_CAPABILITY,
		"image/png",
		"PNG image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_OUT_QUALITY,
		BBT_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (PNGTranslator)"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false},
	{PNG_SETTING_INTERLACE, TRAN_SETTING_INT32, PNG_INTERLACE_NONE}
		// interlacing is off by default
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a PNGTranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						PNGTranslator only publishes one
//						translator, it only returns a
//						PNGTranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new PNGTranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new PNGTranslator();
	else
		return NULL;
}

 /* The png_jmpbuf() macro, used in error handling, became available in
  * libpng version 1.0.6.  If you want to be able to run your code with older
  * versions of libpng, you must define the macro yourself (but only if it
  * is not already defined by libpng!).
  */

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

//// libpng Callback functions!

BPositionIO *
get_pio(png_structp ppng)
{
	BPositionIO *pio = NULL;
	pio = static_cast<BPositionIO *>(png_get_io_ptr(ppng));
	return pio;
}

void
pngcb_read_data(png_structp ppng, png_bytep pdata, png_size_t length)
{
	BPositionIO *pio = get_pio(ppng);
	pio->Read(pdata, static_cast<size_t>(length));
}

void
pngcb_write_data(png_structp ppng, png_bytep pdata, png_size_t length)
{
	BPositionIO *pio = get_pio(ppng);
	pio->Write(pdata, static_cast<size_t>(length));
}

void
pngcb_flush_data(png_structp ppng)
{
	// I don't think I really need to do anything here
}

// ---------------------------------------------------------------
// Constructor
//
// Sets up the version info and the name of the translator so that
// these values can be returned when they are requested.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
PNGTranslator::PNGTranslator()
	: BaseTranslator(B_TRANSLATE("PNG images"), 
		B_TRANSLATE("PNG image translator"),
		PNG_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"PNGTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, B_PNG_FORMAT)
{
}

// ---------------------------------------------------------------
// Destructor
//
// Does nothing
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
PNGTranslator::~PNGTranslator()
{
}

status_t
identify_png_header(BPositionIO *inSource, translator_info *outInfo)
{
	const int32 kSigSize = 8;
	uint8 buf[kSigSize];
	if (inSource->Read(buf, kSigSize) != kSigSize)
		return B_NO_TRANSLATOR;
	if (png_sig_cmp(buf, 0, kSigSize))
		// if first 8 bytes of stream don't match PNG signature bail
		return B_NO_TRANSLATOR;

	if (outInfo) {
		outInfo->type = B_PNG_FORMAT;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = PNG_IN_QUALITY;
		outInfo->capability = PNG_IN_CAPABILITY;
		strcpy(outInfo->MIME, "image/png");
		strncpy(outInfo->name, B_TRANSLATE("PNG image"),
			sizeof(outInfo->name));
	}

	return B_OK;
}

// ---------------------------------------------------------------
// DerivedIdentify
//
// Examines the data from inSource and determines if it is in a
// format that this translator knows how to work with.
//
// Preconditions:
//
// Parameters:	inSource,	where the data to examine is
//
//				inFormat,	a hint about the data in inSource,
//							it is ignored since it is only a hint
//
//				ioExtension,	configuration settings for the
//								translator (not used)
//
//				outInfo,	information about what data is in
//							inSource and how well this translator
//							can handle that data is stored here
//
//				outType,	The format that the user wants
//							the data in inSource to be
//							converted to
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if this translator can't handle
//								the data in inSource
//
// B_ERROR,	if there was an error converting the data to the host
//			format
//
// B_BAD_VALUE, if the settings in ioExtension are bad
//
// B_OK,	if this translator understood the data and there were
//			no errors found
//
// Other errors if BPositionIO::Read() returned an error value
// ---------------------------------------------------------------
status_t
PNGTranslator::DerivedIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	return identify_png_header(inSource, outInfo);
}

status_t
PNGTranslator::translate_from_png_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination)
{
	if (identify_png_header(inSource, NULL) != B_OK)
		return B_NO_TRANSLATOR;

	status_t result = B_ERROR;
		// if a libpng errors before this is set
		// to a different value, the above is what
		// will be returned from this function

	bool bheaderonly = false, bdataonly = false;

	// for storing decoded PNG row data
	uint8 **prows = NULL, *prow = NULL;
	png_uint_32 nalloc = 0;

	png_structp ppng = NULL;
	png_infop pinfo = NULL;
	while (ppng == NULL) {
		// create PNG read pointer with default error handling routines
		ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!ppng)
			break;
		// alocate / init memory for image information
		pinfo = png_create_info_struct(ppng);
		if (!pinfo)
			break;
		// set error handling
		if (setjmp(png_jmpbuf(ppng)))
			// When an error occurs in libpng, it uses
			// the longjmp function to continue execution
			// from this point
			break;

		// set read callback function
		png_set_read_fn(ppng, static_cast<void *>(inSource), pngcb_read_data);

		// Read in PNG image info
		png_set_sig_bytes(ppng, 8);
		png_read_info(ppng, pinfo);

		png_uint_32 width, height;
		int bit_depth, color_type, interlace_type;
		png_get_IHDR(ppng, pinfo, &width, &height, &bit_depth, &color_type,
			&interlace_type, NULL, NULL);

		// Setup image transformations to make converting it easier
		bool balpha = false;

		if (bit_depth == 16)
			png_set_strip_16(ppng);
		else if (bit_depth < 8)
			png_set_packing(ppng);

		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(ppng);

		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
			// In order to convert from low-depth gray images to RGB,
			// I first need to convert them to grayscale, 8 bpp
			png_set_expand_gray_1_2_4_to_8(ppng);

		if (png_get_valid(ppng, pinfo, PNG_INFO_tRNS)) {
			// if there is transparency data in the
			// PNG, but not in the form of an alpha channel
			balpha = true;
			png_set_tRNS_to_alpha(ppng);
		}

		// change RGB to BGR as it is in 'bits'
		if (color_type & PNG_COLOR_MASK_COLOR)
			png_set_bgr(ppng);

		// have libpng convert gray to RGB for me
		if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(ppng);

		if (color_type & PNG_COLOR_MASK_ALPHA)
			// if image contains an alpha channel
			balpha = true;

		if (!balpha)
			// add filler byte for images without alpha
			// so that the pixels are 4 bytes each
			png_set_filler(ppng, 0xff, PNG_FILLER_AFTER);

		// Check that transformed PNG rowbytes matches
		// what is expected
		const int32 kbytes = 4;
		png_uint_32 rowbytes = png_get_rowbytes(ppng, pinfo);
		if (rowbytes < kbytes * width)
			rowbytes = kbytes * width;

		if (!bdataonly) {
			// Write out the data to outDestination
			// Construct and write Be bitmap header
			TranslatorBitmap bitsHeader;
			bitsHeader.magic = B_TRANSLATOR_BITMAP;
			bitsHeader.bounds.left = 0;
			bitsHeader.bounds.top = 0;
			bitsHeader.bounds.right = width - 1;
			bitsHeader.bounds.bottom = height - 1;
			bitsHeader.rowBytes = 4 * width;
			if (balpha)
				bitsHeader.colors = B_RGBA32;
			else
				bitsHeader.colors = B_RGB32;
			bitsHeader.dataSize = bitsHeader.rowBytes * height;
			if (swap_data(B_UINT32_TYPE, &bitsHeader,
				sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
				result = B_ERROR;
				break;
			}
			outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));

			if (bheaderonly) {
				result = B_OK;
				break;
			}
		}

		if (interlace_type == PNG_INTERLACE_NONE) {
			// allocate buffer for storing PNG row
			prow = new uint8[rowbytes];
			if (!prow) {
				result = B_NO_MEMORY;
				break;
			}
			for (png_uint_32 i = 0; i < height; i++) {
				png_read_row(ppng, prow, NULL);
				outDestination->Write(prow, width * kbytes);
			}
			result = B_OK;
				// Set OK status here, because, in the event of
				// an error, png_read_end() will longjmp to error
				// handler above and not execute lines below it

			// finish reading, pass NULL for info because I
			// don't need the extra data
			png_read_end(ppng, NULL);

			break;

		} else {
			// interlaced PNG image
			prows = new uint8 *[height];
			if (!prows) {
				result = B_NO_MEMORY;
				break;
			}
			// allocate enough memory to store the whole image
			for (nalloc = 0; nalloc < height; nalloc++) {
				prows[nalloc] = new uint8[rowbytes];
				if (!prows[nalloc])
					break;
			}

			if (nalloc < height)
				result = B_NO_MEMORY;
			else {
				png_read_image(ppng, prows);

				for (png_uint_32 i = 0; i < height; i++)
					outDestination->Write(prows[i], width * kbytes);
				result = B_OK;
					// Set OK status here, because, in the event of
					// an error, png_read_end() will longjmp to error
					// handler above and not execute lines below it

				png_read_end(ppng, NULL);
			}

			break;
		}
	}

	if (ppng) {
		delete[] prow;
		prow = NULL;

		// delete row pointers and array of pointers to rows
		while (nalloc) {
			nalloc--;
			delete[] prows[nalloc];
		}
		delete[] prows;
		prows = NULL;

		// free PNG handle / info structures
		if (!pinfo)
			png_destroy_read_struct(&ppng, NULL, NULL);
		else
			png_destroy_read_struct(&ppng, &pinfo, NULL);
	}

	return result;
}

status_t
PNGTranslator::translate_from_png(BPositionIO *inSource, uint32 outType,
	BPositionIO *outDestination)
{
	if (outType == B_TRANSLATOR_BITMAP)
		return translate_from_png_to_bits(inSource, outDestination);
	else {
		// Translate from PNG to PNG
		translate_direct_copy(inSource, outDestination);
		return B_OK;
	}
}

// Convert width pixels from pbits to PNG format, storing the
// result in ppng
status_t
pix_bits_to_png(uint8 *pbits, uint8 *ppng, color_space fromspace,
	uint32 width, const color_map *pmap, int32 bitsBytesPerPixel)
{
	status_t bytescopied = 0;
	uint16 val;

	switch (fromspace) {
		case B_RGBA32:
			bytescopied = width * bitsBytesPerPixel;
			memcpy(ppng, pbits, bytescopied);
			break;

		case B_RGB32:
		case B_RGB24:
			bytescopied = width * bitsBytesPerPixel;
			while (width--) {
				memcpy(ppng, pbits, 3);
				ppng += 3;
				pbits += bitsBytesPerPixel;
			}
			break;

		case B_RGBA32_BIG:
			bytescopied = width * 4;
			while (width--) {
				ppng[0] = pbits[3];
				ppng[1] = pbits[2];
				ppng[2] = pbits[1];
				ppng[3] = pbits[0];

				ppng += 4;
				pbits += 4;
			}
			break;

		case B_CMYA32:
			bytescopied = width * 4;
			while (width--) {
				ppng[0] = 255 - pbits[2];
				ppng[1] = 255 - pbits[1];
				ppng[2] = 255 - pbits[0];
				ppng[3] = pbits[3];

				ppng += 4;
				pbits += 4;
			}
			break;

		case B_CMYK32:
		{
			int32 comp;
			bytescopied = width * 3;
			while (width--) {
				comp = 255 - pbits[2] - pbits[3];
				ppng[0] = (comp < 0) ? 0 : comp;

				comp = 255 - pbits[1] - pbits[3];
				ppng[1] = (comp < 0) ? 0 : comp;

				comp = 255 - pbits[0] - pbits[3];
				ppng[2] = (comp < 0) ? 0 : comp;

				ppng += 3;
				pbits += 4;
			}
			break;
		}

		case B_CMY32:
		case B_CMY24:
			bytescopied = width * 3;
			while (width--) {
				ppng[0] = 255 - pbits[2];
				ppng[1] = 255 - pbits[1];
				ppng[2] = 255 - pbits[0];

				ppng += 3;
				pbits += bitsBytesPerPixel;
			}
			break;

		case B_RGB16:
		case B_RGB16_BIG:
			bytescopied = width * 3;
			while (width--) {
				if (fromspace == B_RGB16)
					val = pbits[0] + (pbits[1] << 8);
				else
					val = pbits[1] + (pbits[0] << 8);

				ppng[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				ppng[1] =
					((val & 0x7e0) >> 3) | ((val & 0x7e0) >> 9);
				ppng[2] =
					((val & 0xf800) >> 8) | ((val & 0xf800) >> 13);

				ppng += 3;
				pbits += 2;
			}
			break;

		case B_RGB15:
		case B_RGB15_BIG:
			bytescopied = width * 3;
			while (width--) {
				if (fromspace == B_RGB15)
					val = pbits[0] + (pbits[1] << 8);
				else
					val = pbits[1] + (pbits[0] << 8);
				ppng[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				ppng[1] =
					((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				ppng[2] =
					((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);

				ppng += 3;
				pbits += 2;
			}
			break;

		case B_RGBA15:
		case B_RGBA15_BIG:
			bytescopied = width * 4;
			while (width--) {
				if (fromspace == B_RGBA15)
					val = pbits[0] + (pbits[1] << 8);
				else
					val = pbits[1] + (pbits[0] << 8);
				ppng[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				ppng[1] =
					((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				ppng[2] =
					((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
				ppng[3] = (val & 0x8000) ? 255 : 0;

				ppng += 4;
				pbits += 2;
			}
			break;

		case B_RGB32_BIG:
			bytescopied = width * 3;
			while (width--) {
				ppng[0] = pbits[3];
				ppng[1] = pbits[2];
				ppng[2] = pbits[1];

				ppng += 3;
				pbits += 4;
			}
			break;

		case B_RGB24_BIG:
			bytescopied = width * 3;
			while (width--) {
				ppng[0] = pbits[2];
				ppng[1] = pbits[1];
				ppng[2] = pbits[0];

				ppng += 3;
				pbits += 3;
			}
			break;

		case B_CMAP8:
		{
			rgb_color c;
			bytescopied = width * 3;
			while (width--) {
				c = pmap->color_list[pbits[0]];
				ppng[0] = c.blue;
				ppng[1] = c.green;
				ppng[2] = c.red;

				ppng += 3;
				pbits++;
			}
			break;
		}

		case B_GRAY8:
			bytescopied = width;
			memcpy(ppng, pbits, bytescopied);
			break;

		default:
			bytescopied = B_ERROR;
			break;
	} // switch (fromspace)

	return bytescopied;
}

status_t
PNGTranslator::translate_from_bits_to_png(BPositionIO *inSource,
	BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;

	status_t result;

	result = identify_bits_header(inSource, NULL, &bitsHeader);
	if (result != B_OK)
		return result;

	const color_map *pmap = NULL;
	if (bitsHeader.colors == B_CMAP8) {
		pmap = system_colors();
		if (!pmap)
			return B_ERROR;
	}

	png_uint_32 width, height;
	width = static_cast<png_uint_32>(bitsHeader.bounds.Width() + 1);
	height = static_cast<png_uint_32>(bitsHeader.bounds.Height() + 1);

	int32 pngBytesPerPixel = 0;
	int bit_depth, color_type, interlace_type;
	bit_depth = 8;
	switch (bitsHeader.colors) {
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_CMYA32:
		case B_RGBA15:
		case B_RGBA15_BIG:
			pngBytesPerPixel = 4;
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			break;

		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGB24:
		case B_RGB24_BIG:
		case B_CMY32:
		case B_CMYK32:
		case B_CMY24:
		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGB15:
		case B_RGB15_BIG:
			pngBytesPerPixel = 3;
			color_type = PNG_COLOR_TYPE_RGB;
			break;

		// ADD SUPPORT FOR B_CMAP8 HERE (later)

		case B_GRAY8:
			pngBytesPerPixel = 1;
			color_type = PNG_COLOR_TYPE_GRAY;
			break;

		default:
			return B_NO_TRANSLATOR;
	}
	interlace_type = fSettings->SetGetInt32(PNG_SETTING_INTERLACE);

	int32 bitsBytesPerPixel = 0;
	switch (bitsHeader.colors) {
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_RGB32:
		case B_RGB32_BIG:
		case B_CMYA32:
		case B_CMYK32:
		case B_CMY32:
			bitsBytesPerPixel = 4;
			break;

		case B_RGB24:
		case B_RGB24_BIG:
		case B_CMY24:
			bitsBytesPerPixel = 3;
			break;

		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGBA15:
		case B_RGBA15_BIG:
		case B_RGB15:
		case B_RGB15_BIG:
			bitsBytesPerPixel = 2;
			break;

		case B_GRAY8:
		case B_CMAP8:
			bitsBytesPerPixel = 1;
			break;

		default:
			return B_NO_TRANSLATOR;
	};

	uint8 *pbitsrow = NULL, *prow = NULL;
		// row buffers
	// image buffer for writing whole png image at once
	uint8 **prows = NULL;
	png_uint_32 nalloc = 0;

	png_structp ppng = NULL;
	png_infop pinfo = NULL;

	result = B_NO_TRANSLATOR;
	while (ppng == NULL) {
		// create PNG read pointer with default error handling routines
		ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
			NULL, NULL);
		if (!ppng) {
			result = B_ERROR;
			break;
		}
		// alocate / init memory for image information
		pinfo = png_create_info_struct(ppng);
		if (!pinfo) {
			result = B_ERROR;
			break;
		}
		// set error handling
		if (setjmp(png_jmpbuf(ppng))) {
			// When an error occurs in libpng, it uses
			// the longjmp function to continue execution
			// from this point
			result = B_ERROR;
			break;
		}

		png_set_write_fn(ppng, static_cast<void *>(outDestination),
			pngcb_write_data, pngcb_flush_data);

		// Allocate memory needed to buffer image data
		pbitsrow = new uint8[bitsHeader.rowBytes];
		if (!pbitsrow) {
			result = B_NO_MEMORY;
			break;
		}
		if (interlace_type == PNG_INTERLACE_NONE) {
			prow = new uint8[width * pngBytesPerPixel];
			if (!prow) {
				result = B_NO_MEMORY;
				break;
			}
		} else {
			prows = new uint8 *[height];
			if (!prows) {
				result = B_NO_MEMORY;
				break;
			}
			// allocate enough memory to store the whole image
			for (nalloc = 0; nalloc < height; nalloc++) {
				prows[nalloc] = new uint8[width * pngBytesPerPixel];
				if (!prows[nalloc])
					break;
			}
			if (nalloc < height) {
				result = B_NO_MEMORY;
				// clear out rest of the pointers,
				// so we don't call delete[] with invalid pointers
				for (; nalloc < height; nalloc++)
					prows[nalloc] = NULL;
				break;
			}
		}

		// Specify image info
		png_set_IHDR(ppng, pinfo, width, height, bit_depth, color_type,
			interlace_type, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(ppng, pinfo);

		png_set_bgr(ppng);

		// write out image data
		if (interlace_type == PNG_INTERLACE_NONE) {
			for (png_uint_32 i = 0; i < height; i++) {
				inSource->Read(pbitsrow, bitsHeader.rowBytes);

				pix_bits_to_png(pbitsrow, prow, bitsHeader.colors, width,
					pmap, bitsBytesPerPixel);

				png_write_row(ppng, prow);
			}
		} else {
			for (png_uint_32 i = 0; i < height; i++) {
				inSource->Read(pbitsrow, bitsHeader.rowBytes);

				pix_bits_to_png(pbitsrow, prows[i], bitsHeader.colors, width,
					pmap, bitsBytesPerPixel);
			}
			png_write_image(ppng, prows);
		}
		png_write_end(ppng, NULL);

		result = B_OK;
		break;
	}

	if (ppng) {
		delete[] pbitsrow;
		pbitsrow = NULL;

		delete[] prow;
		prow = NULL;

		// delete row pointers and array of pointers to rows
		while (nalloc) {
			nalloc--;
			delete[] prows[nalloc];
		}
		delete[] prows;
		prows = NULL;

		// free PNG handle / info structures
		if (!pinfo)
			png_destroy_write_struct(&ppng, NULL);
		else
			png_destroy_write_struct(&ppng, &pinfo);
	}

	return result;
}

// ---------------------------------------------------------------
// DerivedTranslate
//
// Translates the data in inSource to the type outType and stores
// the translated data in outDestination.
//
// Preconditions:
//
// Parameters:	inSource,	the data to be translated
//
//				inInfo,	hint about the data in inSource (not used)
//
//				ioExtension,	configuration options for the
//								translator
//
//				outType,	the type to convert inSource to
//
//				outDestination,	where the translated data is
//								put
//
//				baseType, indicates whether inSource is in the
//				          bits format, not in the bits format or
//				          is unknown
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if the options in ioExtension are bad
//
// B_NO_TRANSLATOR, if this translator doesn't understand the data
//
// B_ERROR, if there was an error allocating memory or converting
//          data
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
PNGTranslator::DerivedTranslate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
	BPositionIO *outDestination, int32 baseType)
{
	if (baseType == 1)
		// if inSource is in bits format
		return translate_from_bits_to_png(inSource, outDestination);
	else if (baseType == 0)
		// if inSource is NOT in bits format
		return translate_from_png(inSource, outType, outDestination);
	else
		return B_NO_TRANSLATOR;
}

BView *
PNGTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new PNGView(BRect(0, 0, PNG_VIEW_WIDTH, PNG_VIEW_HEIGHT),
		B_TRANSLATE("PNGTranslator Settings"), B_FOLLOW_ALL, 
		B_WILL_DRAW, settings);
}

