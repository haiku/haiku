/*****************************************************************************/
// SGITranslator
// Written by Stephan Aßmus
// based on TIFFTranslator written mostly by
// Michael Wilber, OBOS Translation Kit Team
//
// SGITranslator.cpp
//
// This BTranslator based object is for opening and writing 
// SGI images.
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

#include <OS.h>

#include "SGIImage.h"
#include "SGITranslator.h"
#include "SGITranslatorSettings.h"
#include "SGIView.h"

// The input formats that this translator supports.
translation_format gInputFormats[] = {
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
translation_format gOutputFormats[] = {
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
	:	BTranslator(),
		fSettings(new SGITranslatorSettings())
{
	fSettings->LoadSettings();
		// load settings from the SGI Translator settings file

	strcpy(fName, "SGI Images");
	sprintf(fInfo, "SGI image translator v%d.%d.%d %s",
		static_cast<int>(SGI_TRANSLATOR_VERSION >> 8),
		static_cast<int>((SGI_TRANSLATOR_VERSION >> 4) & 0xf),
		static_cast<int>(SGI_TRANSLATOR_VERSION & 0xf), __DATE__);
}

// ---------------------------------------------------------------
// Destructor
//
// releases the settings object
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
	fSettings->Release();
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
SGITranslator::TranslatorName() const
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
SGITranslator::TranslatorInfo() const
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
SGITranslator::TranslatorVersion() const
{
	return SGI_TRANSLATOR_VERSION;
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
SGITranslator::InputFormats(int32 *out_count) const
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
SGITranslator::OutputFormats(int32 *out_count) const
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

// TODO: supress unwanted colorspaces here already?
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
		strcpy(outInfo->name, "Be Bitmap Format (SGITranslator)");
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
identify_sgi_header(BPositionIO *inSource, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType,
	SGIImage **poutSGIImage = NULL)
{
	// Can only output to bits for now
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;
	
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
			strcpy(outInfo->name, "SGI image");
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
SGITranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != SGI_FORMAT)
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
	uint8 ch[4];
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
	// Read settings from ioExtension
	if (ioExtension && fSettings->LoadSettings(ioExtension) < B_OK)
		return B_BAD_VALUE; // reason could be invalid settings,
							// like header only and data only set at the same time
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return identify_bits_header(inSource, outInfo, 4, ch);
	// Might be SGI image
	else
		return identify_sgi_header(inSource, ioExtension, outInfo, outType);
}

// translate_from_bits
status_t
translate_from_bits(BPositionIO *inSource, ssize_t amtread, uint8 *read,
	BMessage *ioExtension, uint32 outType, BPositionIO *outDestination,
	SGITranslatorSettings &settings)
{
	TranslatorBitmap bitsHeader;

	bool bheaderonly = false, bdataonly = false;
	uint32 compression = settings.SetGetCompression();

	status_t ret = identify_bits_header(inSource, NULL, amtread, read, &bitsHeader);
	if (ret < B_OK)
		return ret;

	// Translate B_TRANSLATOR_BITMAP to B_TRANSLATOR_BITMAP, easy enough :)
	if (outType == B_TRANSLATOR_BITMAP) {
		// write out bitsHeader (only if configured to)
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			if (swap_data(B_UINT32_TYPE, &bitsHeader,
				sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK)
				return B_ERROR;
			if (outDestination->Write(&bitsHeader,
				sizeof(TranslatorBitmap)) != sizeof(TranslatorBitmap))
				return B_ERROR;
		}
		
		// write out the data (only if configured to)
		if (bdataonly || (!bheaderonly && !bdataonly)) {
			uint32 size = 4096;
			uint8* buf = new uint8[size];
			uint32 remaining = B_BENDIAN_TO_HOST_INT32(bitsHeader.dataSize);
			ssize_t rd, writ = B_ERROR;
			rd = inSource->Read(buf, size);
			while (rd > 0) {
				writ = outDestination->Write(buf, rd);
				if (writ < 0)
					break;
				remaining -= static_cast<uint32>(writ);
				rd = inSource->Read(buf, min_c(size, remaining));
			}
			delete[] buf;
		
			if (remaining > 0)
				// writ may contain a more specific error
				return writ < 0 ? writ : B_ERROR;
			else
				return B_OK;
		} else
			return B_OK;
		
	// Translate B_TRANSLATOR_BITMAP to SGI_FORMAT
	} else if (outType == SGI_FORMAT) {

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
printf("WriteRow() returned %s!\n", strerror(ret));
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
translate_from_sgi(BPositionIO *inSource, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination,
	SGITranslatorSettings &settings)
{
	status_t ret = B_NO_TRANSLATOR;

	// variables needing cleanup
	SGIImage* sgiImage = NULL;
	
	ret = identify_sgi_header(inSource, ioExtension, NULL, outType, &sgiImage);

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
				ret = outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
		}
if (ret < B_OK)
printf("error writing bits header: %s\n", strerror(ret));
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
SGITranslator::Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != SGI_FORMAT)
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
	uint8 ch[4];
	inSource->Seek(0, SEEK_SET);
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
		
	// Read settings from ioExtension
	if (ioExtension && fSettings->LoadSettings(ioExtension) < B_OK)
		return B_BAD_VALUE;
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	if (n32ch == nbits) {
		// B_TRANSLATOR_BITMAP type
		return translate_from_bits(inSource, 4, ch, ioExtension, outType,
								   outDestination, *fSettings);
	} else
		// Might be SGI image
		return translate_from_sgi(inSource, ioExtension, outType,
								  outDestination, *fSettings);
}

// returns the current translator settings into ioExtension
status_t
SGITranslator::GetConfigurationMessage(BMessage *ioExtension)
{
	return fSettings->GetConfigurationMessage(ioExtension);
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
SGITranslator::MakeConfigurationView(BMessage *ioExtension, BView **outView,
	BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;
	if (ioExtension && fSettings->LoadSettings(ioExtension) < B_OK)
		return B_BAD_VALUE;

	SGIView *view = new SGIView(BRect(0, 0, 225, 175),
		"SGITranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW,
		AcquireSettings());
	if (!view)
		return B_NO_MEMORY;

	*outView = view;
	*outExtent = view->Bounds();

	return B_OK;
}

// AcquireSettings
SGITranslatorSettings *
SGITranslator::AcquireSettings()
{
	return fSettings->Acquire();
}

