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
// Copyright (c) 2003 OpenBeOS Project
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

#include <stdio.h>
#include <string.h>
#include <OS.h>
#include <png.h>
#include "PNGTranslator.h"
#include "PNGView.h"

// The input formats that this translator supports.
translation_format gInputFormats[] = {
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
translation_format gOutputFormats[] = {
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
	if (!pio)
		debugger("pio is NULL");
		
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
	:	BTranslator()
{
	strcpy(fName, "PNG Images");
	sprintf(fInfo, "PNG image translator v%d.%d.%d %s",
		PNG_TRANSLATOR_VERSION / 100, (PNG_TRANSLATOR_VERSION / 10) % 10,
		PNG_TRANSLATOR_VERSION % 10, __DATE__);
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

// ---------------------------------------------------------------
// TranslatorName
//
// Returns the short name of the translator.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: a const char * to the short name of the translator
// ---------------------------------------------------------------	
const char *
PNGTranslator::TranslatorName() const
{
	return fName;
}

// ---------------------------------------------------------------
// TranslatorInfo
//
// Returns a more verbose name for the translator than the one
// TranslatorName() returns. This usually includes version info.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: a const char * to the verbose name of the translator
// ---------------------------------------------------------------
const char *
PNGTranslator::TranslatorInfo() const
{
	return fInfo;
}

// ---------------------------------------------------------------
// TranslatorVersion
//
// Returns the integer representation of the current version of
// this translator.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
int32 
PNGTranslator::TranslatorVersion() const
{
	return PNG_TRANSLATOR_VERSION;
}

// ---------------------------------------------------------------
// InputFormats
//
// Returns a list of input formats supported by this translator.
//
// Preconditions:
//
// Parameters:	out_count,	The number of input formats
//							support is returned here.
//
// Postconditions:
//
// Returns: the list of input formats and the number of input
// formats through the out_count parameter, if out_count is NULL,
// NULL is returned
// ---------------------------------------------------------------
const translation_format *
PNGTranslator::InputFormats(int32 *out_count) const
{
	if (out_count) {
		*out_count = sizeof(gInputFormats) /
			sizeof(translation_format);
		return gInputFormats;
	} else
		return NULL;
}

// ---------------------------------------------------------------
// OutputFormats
//
// Returns a list of output formats supported by this translator.
//
// Preconditions:
//
// Parameters:	out_count,	The number of output formats
//							support is returned here.
//
// Postconditions:
//
// Returns: the list of output formats and the number of output
// formats through the out_count parameter, if out_count is NULL,
// NULL is returned
// ---------------------------------------------------------------	
const translation_format *
PNGTranslator::OutputFormats(int32 *out_count) const
{
	if (out_count) {
		*out_count = sizeof(gOutputFormats) /
			sizeof(translation_format);
		return gOutputFormats;
	} else
		return NULL;
}

// ---------------------------------------------------------------
// identify_bits_header
//
// Determines if the data in inSource is in the
// B_TRANSLATOR_BITMAP ('bits') format. If it is, it returns 
// info about the data in inSource to outInfo and pheader.
//
// Preconditions:
//
// Parameters:	inSource,	The source of the image data
//
//				outInfo,	Information about the translator
//							is copied here
//
//				amtread,	Amount of data read from inSource
//							before this function was called
//
//				read,		Pointer to the data that was read
// 							in before this function was called
//
//				pheader,	The bits header is copied here after
//							it is read in from inSource
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data does not look like
//								bits format data
//
// B_ERROR,	if the header data could not be converted to host
//			format
//
// B_OK,	if the data looks like bits data and no errors were
//			encountered
// ---------------------------------------------------------------
status_t 
identify_bits_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, uint8 *read, TranslatorBitmap *pheader = NULL)
{
	TranslatorBitmap header;
		
	memcpy(&header, read, amtread);
		// copy portion of header already read in
	// read in the rest of the header
	ssize_t size = sizeof(TranslatorBitmap) - amtread;
	if (inSource->Read(
		(reinterpret_cast<uint8 *> (&header)) + amtread, size) != size)
		return B_NO_TRANSLATOR;
		
	// convert to host byte order
	if (swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
		
	// check if header values are reasonable
	if (header.colors != B_RGB32 &&
		header.colors != B_RGB32_BIG &&
		header.colors != B_RGBA32 &&
		header.colors != B_RGBA32_BIG &&
		header.colors != B_RGB24 &&
		header.colors != B_RGB24_BIG &&
		header.colors != B_RGB16 &&
		header.colors != B_RGB16_BIG &&
		header.colors != B_RGB15 &&
		header.colors != B_RGB15_BIG &&
		header.colors != B_RGBA15 &&
		header.colors != B_RGBA15_BIG &&
		header.colors != B_CMAP8 &&
		header.colors != B_GRAY8 &&
		header.colors != B_GRAY1 &&
		header.colors != B_CMYK32 &&
		header.colors != B_CMY32 &&
		header.colors != B_CMYA32 &&
		header.colors != B_CMY24)
		return B_NO_TRANSLATOR;
	if (header.rowBytes * (header.bounds.Height() + 1) != header.dataSize)
		return B_NO_TRANSLATOR;
			
	if (outInfo) {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = BBT_IN_QUALITY;
		outInfo->capability = BBT_IN_CAPABILITY;
		strcpy(outInfo->name, "Be Bitmap Format (PNGTranslator)");
		strcpy(outInfo->MIME, "image/x-be-bitmap");
	}
	
	if (pheader) {
		pheader->magic = header.magic;
		pheader->bounds = header.bounds;
		pheader->rowBytes = header.rowBytes;
		pheader->colors = header.colors;
		pheader->dataSize = header.dataSize;
	}
	
	return B_OK;
}

status_t
identify_png_header(BPositionIO *inSource, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType, ssize_t amtread, uint8 *read)
{
	// Can only output to bits for now
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;
	if (amtread != 8)
		return B_ERROR;
		
	if (!png_check_sig(read, amtread))
		// if first 8 bytes of stream don't match PNG signature bail
		return B_NO_TRANSLATOR;

	if (outInfo) {
		outInfo->type = B_PNG_FORMAT;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = PNG_IN_QUALITY;
		outInfo->capability = PNG_IN_CAPABILITY;
		strcpy(outInfo->MIME, "image/png");
		strcpy(outInfo->name, "PNG image");
	}
		
	return B_OK;
}
	
// ---------------------------------------------------------------
// Identify
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
PNGTranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_PNG_FORMAT)
		return B_NO_TRANSLATOR;
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	uint32 nbits = B_TRANSLATOR_BITMAP;
	if (swap_data(B_UINT32_TYPE, &nbits, sizeof(uint32),
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
	uint8 ch[8];
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
		
	// Read settings from ioExtension
	bool bheaderonly = false, bdataonly = false;
	if (ioExtension) {
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderonly))
			// if failed, make sure bool is default value
			bheaderonly = false;
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataonly))
			// if failed, make sure bool is default value
			bdataonly = false;
			
		if (bheaderonly && bdataonly)
			// can't both "only write the header" and "only write the data"
			// at the same time
			return B_BAD_VALUE;
	}
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return identify_bits_header(inSource, outInfo, 4, ch);
		
	// Might be PNG image
	else {
		if (inSource->Read(ch + 4, 4) != 4)
			return B_NO_TRANSLATOR;
		return identify_png_header(inSource, ioExtension, outInfo, outType, 8, ch);
	}
}

status_t
translate_from_png(BPositionIO *inSource, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination, ssize_t amtread, uint8 *read,
	bool bheaderonly, bool bdataonly)
{
	if (amtread != 8)
		return B_ERROR;

	status_t result = B_NO_TRANSLATOR;
	
	png_structp ppng = NULL;
	png_infop pinfo = NULL;
	while (ppng == NULL) {
		// create PNG read pointer with default error handling routines
		ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
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
			result = B_ERROR;
			break;
		}
		
		// set read callback function
		png_set_read_fn(ppng, static_cast<void *>(inSource), pngcb_read_data);
		
		// Read in PNG image info
		png_set_sig_bytes(ppng, amtread);
		png_read_info(ppng, pinfo);
			
		png_uint_32 width, height;
		int bit_depth, color_type, interlace_type;
		png_get_IHDR(ppng, pinfo, &width, &height, &bit_depth, &color_type,
			&interlace_type, int_p_NULL, int_p_NULL);
		
		// Setup image transformations to make converting it easier
		bool balpha = false;
		
		if (bit_depth == 16)
			png_set_strip_16(ppng);
		else if (bit_depth < 8)
			png_set_packing(ppng);
			
		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(ppng);
			
		if (png_get_valid(ppng, pinfo, PNG_INFO_tRNS)) {
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
		
		if (interlace_type == PNG_INTERLACE_NONE) {
			// allocate buffer for storing PNG row
			png_bytep prow = new png_byte[rowbytes];
			if (!prow) {
				result = B_NO_MEMORY;
				break;
			}
			for (png_uint_32 i = 0; i < height; i++) {
				png_read_row(ppng, prow, NULL);
				outDestination->Write(prow, width * kbytes);
			}
			
			// finish reading, pass NULL for info because I 
			// don't need the extra data
			png_read_end(ppng, NULL);
			delete[] prow;
			prow = NULL;
			
			result = B_OK;
			break;
			
		} else {
			// interlaced PNG image
			png_bytep *prows = new png_bytep[height];
			if (!prows) {
				result = B_NO_MEMORY;
				break;
			}
			// allocate enough memory to store the whole image
			png_uint_32 n;
			for (n = 0; n < height; n++) {
				prows[n] = new png_byte[rowbytes];
				if (!prows[n])
					break;
			}
			
			if (n < height)
				result = B_NO_MEMORY;
			else {
				png_read_image(ppng, prows);
				for (png_uint_32 i = 0; i < height; i++)
					outDestination->Write(prows[i], width * kbytes);
				
				result = B_OK;
			}
			
			// delete row pointers and array of pointers to rows
			while (n) {
				n--;
				delete[] prows[n];
			}
			delete[] prows;
			prows = NULL;
			
			break;
		}
	}
	
	if (ppng) {
		if (!pinfo)
			png_destroy_read_struct(&ppng, png_infopp_NULL, png_infopp_NULL);
		else
			png_destroy_read_struct(&ppng, &pinfo, png_infopp_NULL);
	}

	return result;
}

// ---------------------------------------------------------------
// Translate
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
PNGTranslator::Translate(BPositionIO *inSource, const translator_info *inInfo,
	BMessage *ioExtension, uint32 outType, BPositionIO *outDestination)
{	
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_PNG_FORMAT)
		return B_NO_TRANSLATOR;
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	uint32 nbits = B_TRANSLATOR_BITMAP;
	if (swap_data(B_UINT32_TYPE, &nbits, sizeof(uint32),
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
	uint8 ch[8];
	inSource->Seek(0, SEEK_SET);
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
		
	// Read settings from ioExtension
	bool bheaderonly = false, bdataonly = false;
	if (ioExtension) {
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderonly))
			// if failed, make sure bool is default value
			bheaderonly = false;
		if (ioExtension->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataonly))
			// if failed, make sure bool is default value
			bdataonly = false;
			
		if (bheaderonly && bdataonly)
			// can't both "only write the header" and "only write the data"
			// at the same time
			return B_BAD_VALUE;
	}
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	if (n32ch == nbits) {
		// B_TRANSLATOR_BITMAP type
		outDestination->Write(ch, 4);
		
		const size_t kbufsize = 2048;
		uint8 buffer[kbufsize];
		ssize_t ret = inSource->Read(buffer, kbufsize);
		while (ret > 0) {
			outDestination->Write(buffer, ret);
			ret = inSource->Read(buffer, kbufsize);
		}
		
		return B_OK;
		
	} else {
		// Might be PNG image
		if (inSource->Read(ch + 4, 4) != 4)
			return B_NO_TRANSLATOR;
		if (!png_check_sig(ch, 8))
			return B_NO_TRANSLATOR;

		return translate_from_png(inSource, ioExtension, outType,
			outDestination, 8, ch, bheaderonly, bdataonly);
	}
}

// ---------------------------------------------------------------
// MakeConfigurationView
//
// Makes a BView object for configuring / displaying info about
// this translator. 
//
// Preconditions:
//
// Parameters:	ioExtension,	configuration options for the
//								translator
//
//				outView,		the view to configure the
//								translator is stored here
//
//				outExtent,		the bounds of the view are
//								stored here
//
// Postconditions:
//
// Returns: B_BAD_VALUE if outView or outExtent is NULL,
//			B_NO_MEMORY if the view couldn't be allocated,
//			B_OK if no errors
// ---------------------------------------------------------------
status_t
PNGTranslator::MakeConfigurationView(BMessage *ioExtension, BView **outView,
	BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	PNGView *view = new PNGView(BRect(0, 0, 225, 175),
		"PNGTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!view)
		return B_NO_MEMORY;

	*outView = view;
	*outExtent = view->Bounds();

	return B_OK;
}


