/*****************************************************************************/
// TIFFTranslator
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TIFFTranslator.cpp
//
// This BTranslator based object is for opening and writing 
// TIFF images.
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

#include <string.h>
#include <stdio.h>
#include "TIFFTranslator.h"
#include "TIFFView.h"
#include "TiffIfd.h"

// The input formats that this translator supports.
translation_format gInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		TIFF_IN_QUALITY,
		TIFF_IN_CAPABILITY,
		"image/tiff",
		"TIFF Image"
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
		"Be Bitmap Format (TIFFTranslator)"
	},
	{
		B_TIFF_FORMAT,
		B_TRANSLATOR_BITMAP,
		TIFF_OUT_QUALITY,
		TIFF_OUT_CAPABILITY,
		"image/tiff",
		"TIFF Image"
	}
};

// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a TIFFTranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						TIFFTranslator only publishes one
//						translator, it only returns a
//						TIFFTranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new TIFFTranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new TIFFTranslator();
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
TIFFTranslator::TIFFTranslator()
	:	BTranslator()
{
	strcpy(fName, "TIFF Images");
	sprintf(fInfo, "TIFF image translator v%d.%d.%d %s",
		TIFF_TRANSLATOR_VERSION / 100, (TIFF_TRANSLATOR_VERSION / 10) % 10,
		TIFF_TRANSLATOR_VERSION % 10, __DATE__);
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
TIFFTranslator::~TIFFTranslator()
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
TIFFTranslator::TranslatorName() const
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
TIFFTranslator::TranslatorInfo() const
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
TIFFTranslator::TranslatorVersion() const
{
	return TIFF_TRANSLATOR_VERSION;
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
TIFFTranslator::InputFormats(int32 *out_count) const
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
TIFFTranslator::OutputFormats(int32 *out_count) const
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
		strcpy(outInfo->name, "Be Bitmap Format (TGATranslator)");
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

// If this TIFF has unsupported features,
// return B_NO_TRANSLATOR, otherwise, 
// return B_OK
status_t
check_tiff_fields(TiffIfd &ifd)
{
	uint32 value;
	TiffUintField *pfield = NULL;
	
	// Only the default values are supported for the
	// following fields
	if (ifd.GetUintField(TAG_FILL_ORDER, pfield) == B_OK &&
		pfield->GetUint(value) == B_OK &&
		value != 1)
		return B_NO_TRANSLATOR;
	
	if (ifd.GetUintField(TAG_ORIENTATION, pfield) == B_OK &&
		pfield->GetUint(value) == B_OK &&
		value != 1)
		return B_NO_TRANSLATOR;
	
	if (ifd.GetUintField(TAG_PLANAR_CONFIGURATION, pfield) == B_OK &&
		pfield->GetUint(value) == B_OK &&
		value != 1)
		return B_NO_TRANSLATOR;
		
	if (ifd.GetUintField(TAG_SAMPLE_FORMAT, pfield) == B_OK &&
		pfield->GetUint(value) == B_OK &&
		value != 1)
		return B_NO_TRANSLATOR;
		
	// Only uncompressed images are supported
	if (ifd.GetUintField(TAG_COMPRESSION, pfield) != B_OK)
		return B_NO_TRANSLATOR;
	if (pfield->GetUint(value) != B_OK)
		return B_NO_TRANSLATOR;
	
		
	return B_OK;
}

status_t
identify_tiff_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, uint8 *read, swap_action swp)
{
	if (amtread != 4)
		return B_ERROR;
		
	uint32 firstIFDOffset = 0;
	ssize_t nread = inSource->Read(&firstIFDOffset, 4);
	if (nread != 4) {
		printf("Unable to read first IFD offset\n");
		return B_NO_TRANSLATOR;
	}
	if (swap_data(B_UINT32_TYPE, &firstIFDOffset, sizeof(uint32), swp) != B_OK) {
		printf("swap_data() error\n");
		return B_ERROR;
	}
	printf("First IFD: 0x%.8lx\n", firstIFDOffset);
	TiffIfd ifd(firstIFDOffset, *inSource, swp);
	
	status_t initcheck;
	initcheck = ifd.InitCheck();
	printf("Init Check: %d\n",
		static_cast<int>(initcheck));
		
	// Read in some fields in order to determine whether or not
	// this particular TIFF image is supported by this translator
	
	// Get some stats and print them out
	if (initcheck == B_OK) {
		uint32 value;
		TiffUintField *pfield = NULL;
		
		if (ifd.GetUintField(TAG_IMAGE_WIDTH, pfield) == B_OK) {
			if (pfield->GetUint(value) == B_OK)
				printf("image width: %d\n", value);
		}
		if (ifd.GetUintField(TAG_IMAGE_HEIGHT, pfield) == B_OK) {
			if (pfield->GetUint(value) == B_OK)
				printf("image height: %d\n", value);
		}
		if (ifd.GetUintField(TAG_COMPRESSION, pfield) == B_OK) {
			if (pfield->GetUint(value) == B_OK)
				printf("compression: %d\n", value);
		}
		
		return check_tiff_fields(ifd);
	}
		
	if (initcheck != B_OK && initcheck != B_ERROR && initcheck != B_NO_MEMORY)
		// return B_NO_TRANSLATOR if unexpected data was encountered
		return B_NO_TRANSLATOR;
	else
		// return B_OK, B_ERROR or B_NO_MEMORY
		return initcheck;
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
TIFFTranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_TIFF_FORMAT)
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
		
	// Might be TIFF image
	else {
		// TIFF Byte Order / Magic
		const uint8 kleSig[] = { 0x49, 0x49, 0x2a, 0x00 };
		const uint8 kbeSig[] = { 0x4d, 0x4d, 0x00, 0x2a };
		
		swap_action swp;
		if (memcmp(ch, kleSig, 4) == 0) {
			swp = B_SWAP_LENDIAN_TO_HOST;
			printf("Byte Order: little endian\n");
		} else if (memcmp(ch, kbeSig, 4) == 0) {
			swp = B_SWAP_BENDIAN_TO_HOST;
			printf("Byte Order: big endian\n");		
		} else {
			// If not a TIFF or a Be Bitmap image
			printf("Invalid byte order value\n");
			return B_NO_TRANSLATOR;
		}
		
		return identify_tiff_header(inSource, outInfo, 4, ch, swp);
	}
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
TIFFTranslator::Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_TIFF_FORMAT)
		return B_NO_TRANSLATOR;
		
	return B_NO_TRANSLATOR;
		// translator isn't implemented yet
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
TIFFTranslator::MakeConfigurationView(BMessage *ioExtension, BView **outView,
	BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	TIFFView *view = new TIFFView(BRect(0, 0, 225, 175),
		"TIFFTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!view)
		return B_NO_MEMORY;

	*outView = view;
	*outExtent = view->Bounds();

	return B_OK;
}
