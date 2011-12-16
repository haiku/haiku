/*****************************************************************************/
// SGITranslator
// Written by Stephan Aßmus
// based on TIFFTranslator written mostly by
// Michael Wilber
//
// SGITranslator.cpp
//
// This BTranslator based object is for opening and writing
// SGI images.
//
//
// Copyright (c) 2003-2009 Haiku, Inc. All rights reserved.
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
//
// How this works:
//
// libtiff has a special version of SGIOpen() that gets passed custom
// functions for reading writing etc. and a handle. This handle in our case
// is a BPositionIO object, which libtiff passes on to the functions for reading
// writing etc. So when operations are performed on the SGI* handle that is
// returned by SGIOpen(), libtiff uses the special reading writing etc
// functions so that all stream io happens on the BPositionIO object.

#include <new>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <Catalog.h>
#include <OS.h>

#include "SGIImage.h"
#include "SGITranslator.h"
#include "SGIView.h"

using std::nothrow;

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "SGITranslator"


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (SGITranslator)"
	},
	{
		SGI_FORMAT,
		B_TRANSLATOR_BITMAP,
		SGI_IN_QUALITY,
		SGI_IN_CAPABILITY,
		"image/sgi",
		"SGI image"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_OUT_QUALITY,
		BBT_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (SGITranslator)"
	},
	{
		SGI_FORMAT,
		B_TRANSLATOR_BITMAP,
		SGI_OUT_QUALITY,
		SGI_OUT_CAPABILITY,
		"image/sgi",
		"SGI image"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false},
	{SGI_SETTING_COMPRESSION, TRAN_SETTING_INT32, SGI_COMP_RLE}
		// compression is set to RLE by default
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a SGITranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						SGITranslator only publishes one
//						translator, it only returns a
//						SGITranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new SGITranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new SGITranslator();
	else
		return NULL;
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
SGITranslator::SGITranslator()
	:
	BaseTranslator(B_TRANSLATE("SGI images"), 
		B_TRANSLATE("SGI image translator"),
		SGI_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"SGITranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, SGI_FORMAT)
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
SGITranslator::~SGITranslator()
{
}

status_t
identify_sgi_header(BPositionIO *inSource, translator_info *outInfo, uint32 outType,
	SGIImage **poutSGIImage = NULL)
{
	status_t status = B_NO_MEMORY;
	// construct new SGIImage object and set it to the provided BPositionIO
	SGIImage* sgiImage = new(nothrow) SGIImage();
	if (sgiImage)
		status = sgiImage->SetTo(inSource);

	if (status >= B_OK) {
		if (outInfo) {
			outInfo->type = SGI_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = SGI_IN_QUALITY;
			outInfo->capability = SGI_IN_CAPABILITY;
			strcpy(outInfo->MIME, "image/sgi");
			strlcpy(outInfo->name, B_TRANSLATE("SGI image"),
				sizeof(outInfo->name));
		}
	} else {
		delete sgiImage;
		sgiImage = NULL;
	}
	if (!poutSGIImage)
		// close SGIImage if caller is not interested in SGIImage handle
		delete sgiImage;
	else
		// leave SGIImage open (if it is) and return handle if caller needs it
		*poutSGIImage = sgiImage;

	return status;
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
SGITranslator::DerivedIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	return identify_sgi_header(inSource, outInfo, outType);
}

// translate_from_bits
status_t
SGITranslator::translate_from_bits(BPositionIO *inSource, uint32 outType,
	BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;

	uint32 compression = fSettings->SetGetInt32(SGI_SETTING_COMPRESSION);

	status_t ret = identify_bits_header(inSource, NULL, &bitsHeader);
	if (ret < B_OK)
		return ret;

	// Translate B_TRANSLATOR_BITMAP to SGI_FORMAT
	if (outType == SGI_FORMAT) {

		// common fields which are independent of the bitmap format
		uint32 width = bitsHeader.bounds.IntegerWidth() + 1;
		uint32 height = bitsHeader.bounds.IntegerHeight() + 1;
		uint32 bytesPerRow = bitsHeader.rowBytes;
		uint32 bytesPerChannel = 1;
		color_space format = bitsHeader.colors;

		uint32 channelCount;
		switch (format) {
			case B_GRAY8:
				channelCount = 1;
				break;
			case B_RGB32:
			case B_RGB32_BIG:
			case B_RGB24:
			case B_RGB24_BIG:
				channelCount = 3;
				break;
			case B_RGBA32:
			case B_RGBA32_BIG:
				channelCount = 4;
				break;
			default:
				return B_NO_TRANSLATOR;
		}

		// Set up SGI header
		SGIImage* sgiImage = new SGIImage();
		status_t ret = sgiImage->SetTo(outDestination, width, height,
									   channelCount, bytesPerChannel, compression);
		if (ret >= B_OK) {
			// read one row at a time,
			// convert to the correct format
			// and write out the results

			// SGI Images store each channel separately
			// a buffer is allocated big enough to hold all channels
			// then the pointers are assigned with offsets into that buffer
			uint8** rows = new(nothrow) uint8*[channelCount];
			if (rows)
				rows[0] = new(nothrow) uint8[width * channelCount * bytesPerChannel];
			// rowBuffer is going to hold the converted data
			uint8* rowBuffer = new(nothrow) uint8[bytesPerRow];
			if (rows && rows[0] && rowBuffer) {
				// assign the other pointers (channel offsets in row buffer)
				for (uint32 i = 1; i < channelCount; i++)
					rows[i] = rows[0] + i * width;
				// loop through all lines of the image
				for (int32 y = height - 1; y >= 0 && ret >= B_OK; y--) {

					ret = inSource->Read(rowBuffer, bytesPerRow);
					// see if an error happened while reading
					if (ret < B_OK)
						break;
					// convert to native format (big endian)
					switch (format) {
						case B_GRAY8: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[0];
								src += 1;
							}
							break;
						}
						case B_RGB24: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[2];
								rows[1][x] = src[1];
								rows[2][x] = src[0];
								src += 3;
							}
							break;
						}
						case B_RGB24_BIG: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[0];
								rows[1][x] = src[1];
								rows[2][x] = src[2];
								src += 3;
							}
							break;
						}
						case B_RGB32: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[2];
								rows[1][x] = src[1];
								rows[2][x] = src[0];
								// ignore src[3]
								src += 4;
							}
							break;
						}
						case B_RGB32_BIG: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[1];
								rows[1][x] = src[2];
								rows[2][x] = src[3];
								// ignore src[0]
								src += 4;
							}
							break;
						}
						case B_RGBA32: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[2];
								rows[1][x] = src[1];
								rows[2][x] = src[0];
								rows[3][x] = src[3];
								src += 4;
							}
							break;
						}
						case B_RGBA32_BIG: {
							uint8* src = rowBuffer;
							for (uint32 x = 0; x < width; x++) {
								rows[0][x] = src[1];
								rows[1][x] = src[2];
								rows[2][x] = src[3];
								rows[3][x] = src[0];
								src += 4;
							}
							break;
						}
						default:
							// cannot be here
							break;
					} // switch (format)

					// for each channel, write a row buffer
					for (uint32 z = 0; z < channelCount; z++) {
						ret = sgiImage->WriteRow(rows[z], y, z);
						if (ret < B_OK) {
							syslog(LOG_ERR,
								"WriteRow() returned %s!\n"), strerror(ret);
							break;
						}
					}

				} // for (uint32 y = 0; y < height && ret >= B_OK; y++)
				if (ret >= B_OK)
					ret = B_OK;
			} else // if (rows && rows[0] && rowBuffer)
				ret = B_NO_MEMORY;

			delete[] rows[0];
			delete[] rows;
			delete[] rowBuffer;
		}

		// done with the SGIImage object
		delete sgiImage;

		return ret;
	}
	return B_NO_TRANSLATOR;
}

// translate_from_sgi
status_t
SGITranslator::translate_from_sgi(BPositionIO *inSource, uint32 outType,
	BPositionIO *outDestination)
{
	status_t ret = B_NO_TRANSLATOR;

	// if copying SGI_FORMAT to SGI_FORMAT
	if (outType == SGI_FORMAT) {
		translate_direct_copy(inSource, outDestination);
		return B_OK;
	}

	// variables needing cleanup
	SGIImage* sgiImage = NULL;

	ret = identify_sgi_header(inSource, NULL, outType, &sgiImage);

	if (ret >= B_OK) {

		bool bheaderonly = false, bdataonly = false;

		uint32 width = sgiImage->Width();
		uint32 height = sgiImage->Height();
		uint32 channelCount = sgiImage->CountChannels();
		color_space format = B_RGBA32;
		uint32 bytesPerRow = 0;
		uint32 bytesPerChannel = sgiImage->BytesPerChannel();

		if (channelCount == 1) {
//			format = B_GRAY8;	// this format is not supported by most applications
//			bytesPerRow = width;
			format = B_RGB32;
			bytesPerRow = width * 4;
		} else if (channelCount == 2) {
			// means gray (luminance) + alpha, we convert that to B_RGBA32
			format = B_RGBA32;
			bytesPerRow = width * 4;
		} else if (channelCount == 3) {
			format = B_RGB32; // should be B_RGB24, but let's not push it too hard...
			bytesPerRow = width * 4;
		} else if (channelCount == 4) {
			format = B_RGBA32;
			bytesPerRow = width * 4;
		} else
			ret = B_NO_TRANSLATOR; // we cannot handle this image

		if (ret >= B_OK && !bdataonly) {
			// Construct and write Be bitmap header
			TranslatorBitmap bitsHeader;
			bitsHeader.magic = B_TRANSLATOR_BITMAP;
			bitsHeader.bounds.left = 0;
			bitsHeader.bounds.top = 0;
			bitsHeader.bounds.right = width - 1;
			bitsHeader.bounds.bottom = height - 1;
			bitsHeader.rowBytes = bytesPerRow;
			bitsHeader.colors = format;
			bitsHeader.dataSize = bitsHeader.rowBytes * height;
			if ((ret = swap_data(B_UINT32_TYPE, &bitsHeader,
				sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN)) < B_OK) {
				return ret;
			} else
				ret = outDestination->Write(&bitsHeader, 
					sizeof(TranslatorBitmap));
		}
		if (ret < B_OK)
			syslog(LOG_ERR, "error writing bits header: %s\n", strerror(ret));
		if (ret >= B_OK && !bheaderonly) {
			// read one row at a time,
			// convert to the correct format
			// and write out the results

			// SGI Images store each channel separately
			// a buffer is allocated big enough to hold all channels
			// then the pointers are assigned with offsets into that buffer
			uint8** rows = new(nothrow) uint8*[channelCount];
			if (rows)
				rows[0] = new(nothrow) uint8[width * channelCount * bytesPerChannel];
			// rowBuffer is going to hold the converted data
			uint8* rowBuffer = new(nothrow) uint8[bytesPerRow];
			if (rows && rows[0] && rowBuffer) {
				// assign the other pointers (channel offsets in row buffer)
				for (uint32 i = 1; i < channelCount; i++)
					rows[i] = rows[0] + i * width * bytesPerChannel;
				// loop through all lines of the image
				for (int32 y = height - 1; y >= 0 && ret >= B_OK; y--) {
					// fill the row buffer with each channel
					for (uint32 z = 0; z < channelCount; z++) {
						ret = sgiImage->ReadRow(rows[z], y, z);
						if (ret < B_OK)
							break;
					}
					// see if an error happened while reading
					if (ret < B_OK)
						break;
					// convert to native format (big endian)
					if (bytesPerChannel == 1) {
						switch (format) {
							case B_GRAY8: {
								uint8* dst = rowBuffer;
								for (uint32 x = 0; x < width; x++) {
									dst[0] = rows[0][x];
									dst += 1;
								}
								break;
							}
							case B_RGB24: {
								uint8* dst = rowBuffer;
								for (uint32 x = 0; x < width; x++) {
									dst[0] = rows[2][x];
									dst[1] = rows[1][x];
									dst[2] = rows[0][x];
									dst += 3;
								}
								break;
							}
							case B_RGB32: {
								uint8* dst = rowBuffer;
								if (channelCount == 1) {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows[0][x];
										dst[1] = rows[0][x];
										dst[2] = rows[0][x];
										dst[3] = 255;
										dst += 4;
									}
								} else {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows[2][x];
										dst[1] = rows[1][x];
										dst[2] = rows[0][x];
										dst[3] = 255;
										dst += 4;
									}
								}
								break;
							}
							case B_RGBA32: {
								uint8* dst = rowBuffer;
								if (channelCount == 2) {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows[0][x];
										dst[1] = rows[0][x];
										dst[2] = rows[0][x];
										dst[3] = rows[1][x];
										dst += 4;
									}
								} else {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows[2][x];
										dst[1] = rows[1][x];
										dst[2] = rows[0][x];
										dst[3] = rows[3][x];
										dst += 4;
									}
								}
								break;
							}
							default:
								// cannot be here
								break;
						} // switch (format)
						ret = outDestination->Write(rowBuffer, bytesPerRow);
					} else {
						// support for 16 bits per channel images
						uint16** rows16 = (uint16**)rows;
						switch (format) {
							case B_GRAY8: {
								uint8* dst = rowBuffer;
								for (uint32 x = 0; x < width; x++) {
									dst[0] = rows16[0][x] >> 8;
									dst += 1;
								}
								break;
							}
							case B_RGB24: {
								uint8* dst = rowBuffer;
								for (uint32 x = 0; x < width; x++) {
									dst[0] = rows16[2][x] >> 8;
									dst[1] = rows16[1][x] >> 8;
									dst[2] = rows16[0][x] >> 8;
									dst += 3;
								}
								break;
							}
							case B_RGB32: {
								uint8* dst = rowBuffer;
								if (channelCount == 1) {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows16[0][x] >> 8;
										dst[1] = rows16[0][x] >> 8;
										dst[2] = rows16[0][x] >> 8;
										dst[3] = 255;
										dst += 4;
									}
								} else {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows16[2][x] >> 8;
										dst[1] = rows16[1][x] >> 8;
										dst[2] = rows16[0][x] >> 8;
										dst[3] = 255;
										dst += 4;
									}
								}
								break;
							}
							case B_RGBA32: {
								uint8* dst = rowBuffer;
								if (channelCount == 2) {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows16[0][x] >> 8;
										dst[1] = rows16[0][x] >> 8;
										dst[2] = rows16[0][x] >> 8;
										dst[3] = rows16[1][x] >> 8;
										dst += 4;
									}
								} else {
									for (uint32 x = 0; x < width; x++) {
										dst[0] = rows16[2][x] >> 8;
										dst[1] = rows16[1][x] >> 8;
										dst[2] = rows16[0][x] >> 8;
										dst[3] = rows16[3][x] >> 8;
										dst += 4;
									}
								}
								break;
							}
							default:
								// cannot be here
								break;
						} // switch (format)
						ret = outDestination->Write(rowBuffer, bytesPerRow);
					} // 16 bit version
				} // for (uint32 y = 0; y < height && ret >= B_OK; y++)
				if (ret >= B_OK)
					ret = B_OK;
			} else // if (rows && rows[0] && rowBuffer)
				ret = B_NO_MEMORY;
			delete[] rows[0];
			delete[] rows;
			delete[] rowBuffer;
		} // if (ret >= B_OK && !bheaderonly)
	} // if (ret >= B_OK)
	delete sgiImage;

	return ret;
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
SGITranslator::DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType)
{
	if (baseType == 1)
		// if inSource is in bits format
		return translate_from_bits(inSource, outType, outDestination);
	else if (baseType == 0)
		// if inSource is NOT in bits format
		return translate_from_sgi(inSource, outType, outDestination);
	else
		// if BaseTranslator did not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;
}

BView *
SGITranslator::NewConfigView(TranslatorSettings *settings)
{
	return new SGIView(B_TRANSLATE("SGITranslator Settings"), B_WILL_DRAW, 
		settings);
}
