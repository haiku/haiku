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

#include <stdio.h>
#include <string.h>
#include "TIFFTranslator.h"
#include "TIFFView.h"
#include "TiffIfd.h"
#include "TiffIfdList.h"
#include "StreamBuffer.h"
#include "BitReader.h"

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
		"TIFF image"
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
		"TIFF image"
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

TiffDetails::TiffDetails()
{
	memset(this, 0, sizeof(TiffDetails));
}

TiffDetails::~TiffDetails()
{
	delete[] pstripOffsets;
	delete[] pstripByteCounts;
	delete[] pcolorMap;
	
	memset(this, 0, sizeof(TiffDetails));
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
	fpblackTree = NULL;
	fpwhiteTree = NULL;
	
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
	delete fpblackTree;
	fpblackTree = NULL;
	delete fpwhiteTree;
	fpwhiteTree = NULL;
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
		strcpy(outInfo->name, "Be Bitmap Format (TIFFTranslator)");
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
check_tiff_fields(TiffIfd &ifd, TiffDetails *pdetails, bool bAllocArrays)
{
	if (!pdetails)
		return B_BAD_VALUE;
		
	try {
		// Extra Samples are not yet supported
		if (ifd.HasField(TAG_EXTRA_SAMPLES))
			return B_NO_TRANSLATOR;
			
		// Only the default values are supported for the
		// following fields.  HasField is called so that
		// if a field is not present, a TiffIfdFieldNotFoundException
		// will not be thrown when GetUint is called.
		// (If they are not present, that is fine, but if
		// they are present and have a non-default value,
		// that is a problem)
		if (ifd.HasField(TAG_ORIENTATION) &&
			ifd.GetUint(TAG_ORIENTATION) != 1)
			return B_NO_TRANSLATOR;
		if (ifd.HasField(TAG_PLANAR_CONFIGURATION) &&
			ifd.GetUint(TAG_PLANAR_CONFIGURATION) != 1)
			return B_NO_TRANSLATOR;
		if (ifd.HasField(TAG_SAMPLE_FORMAT) &&
			ifd.GetUint(TAG_SAMPLE_FORMAT) != 1)
			return B_NO_TRANSLATOR;
			
		// Copy fields useful to TIFFTranslator
		uint32 width			= ifd.GetUint(TAG_IMAGE_WIDTH);
		uint32 height			= ifd.GetUint(TAG_IMAGE_HEIGHT);
		uint16 interpretation	= ifd.GetUint(TAG_PHOTO_INTERPRETATION);
		
		uint32 compression;
		if (!ifd.HasField(TAG_COMPRESSION))
			compression = COMPRESSION_NONE;
		else
			compression = ifd.GetUint(TAG_COMPRESSION);
		
		uint32 t4options = 0;
		uint16 imageType = 0, bitsPerPixel = 0, fillOrder = 1;
		switch (interpretation) {
			// black and white or grayscale images
			case PHOTO_WHITEZERO:
			case PHOTO_BLACKZERO:
				// default value for samples per pixel is 1
				if (ifd.HasField(TAG_SAMPLES_PER_PIXEL) &&
					ifd.GetUint(TAG_SAMPLES_PER_PIXEL) != 1)
					return B_NO_TRANSLATOR;
				
				// Only 1D and fill byte boundary t4options are
				// supported
				if (ifd.HasField(TAG_T4_OPTIONS))
					t4options = ifd.GetUint(TAG_T4_OPTIONS);
				if (t4options != 0 && t4options != T4_FILL_BYTE_BOUNDARY)
					return B_NO_TRANSLATOR;
					
				// default value for bits per sample is 1
				if (!ifd.HasField(TAG_BITS_PER_SAMPLE) ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE) == 1) {
					bitsPerPixel = 1;
					
					if (compression != COMPRESSION_NONE &&
						compression != COMPRESSION_HUFFMAN &&
						compression != COMPRESSION_T4 &&
						compression != COMPRESSION_PACKBITS)
						return B_NO_TRANSLATOR;
						
					if (ifd.HasField(TAG_FILL_ORDER)) {
						fillOrder = ifd.GetUint(TAG_FILL_ORDER);
						if (fillOrder != 1 && fillOrder != 2)
							return B_NO_TRANSLATOR;
					}
			
				} else if (ifd.GetUint(TAG_BITS_PER_SAMPLE) == 4 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE) == 8) {
					bitsPerPixel = ifd.GetUint(TAG_BITS_PER_SAMPLE);
					
					if (ifd.HasField(TAG_FILL_ORDER) &&
						ifd.GetUint(TAG_FILL_ORDER) != 1)
						return B_NO_TRANSLATOR;
					if (compression != COMPRESSION_NONE &&
						compression != COMPRESSION_PACKBITS)
						return B_NO_TRANSLATOR;
				} else
					return B_NO_TRANSLATOR;
					
				imageType = TIFF_BILEVEL;
					
				break;
				
			// Palette color images
			case PHOTO_PALETTE:
				if (ifd.HasField(TAG_FILL_ORDER) &&
					ifd.GetUint(TAG_FILL_ORDER) != 1)
					return B_NO_TRANSLATOR;
				if (compression != COMPRESSION_NONE &&
					compression != COMPRESSION_PACKBITS)
					return B_NO_TRANSLATOR;
					
				// default value for samples per pixel is 1
				if (ifd.HasField(TAG_SAMPLES_PER_PIXEL) &&
					ifd.GetUint(TAG_SAMPLES_PER_PIXEL) != 1)
					return B_NO_TRANSLATOR;
					
				bitsPerPixel = ifd.GetUint(TAG_BITS_PER_SAMPLE);
				if (bitsPerPixel != 4 && bitsPerPixel != 8)
					return B_NO_TRANSLATOR;
					
				// ensure that the color map is the expected length
				if (ifd.GetCount(TAG_COLOR_MAP) !=
					(uint32) (3 * (1 << bitsPerPixel)))
					return B_NO_TRANSLATOR;
			
				imageType = TIFF_PALETTE;
				break;
				
			case PHOTO_RGB:
				if (ifd.HasField(TAG_FILL_ORDER) &&
					ifd.GetUint(TAG_FILL_ORDER) != 1)
					return B_NO_TRANSLATOR;
				if (compression != COMPRESSION_NONE &&
					compression != COMPRESSION_PACKBITS)
					return B_NO_TRANSLATOR;
					
				if (ifd.GetUint(TAG_SAMPLES_PER_PIXEL) != 3)
					return B_NO_TRANSLATOR;
				if (ifd.GetCount(TAG_BITS_PER_SAMPLE) != 3 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 1) != 8 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 2) != 8 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 3) != 8)
					return B_NO_TRANSLATOR;
				
				imageType = TIFF_RGB;
				break;
				
			case PHOTO_SEPARATED:
				if (ifd.HasField(TAG_FILL_ORDER) &&
					ifd.GetUint(TAG_FILL_ORDER) != 1)
					return B_NO_TRANSLATOR;
				if (compression != COMPRESSION_NONE &&
					compression != COMPRESSION_PACKBITS)
					return B_NO_TRANSLATOR;
					
				// CMYK (default ink set)
				// is the only ink set supported
				if (ifd.HasField(TAG_INK_SET) &&
					ifd.GetUint(TAG_INK_SET) != INK_SET_CMYK)
					return B_NO_TRANSLATOR;
				
				if (ifd.GetUint(TAG_SAMPLES_PER_PIXEL) != 4)
					return B_NO_TRANSLATOR;
				if (ifd.GetCount(TAG_BITS_PER_SAMPLE) != 4 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 1) != 8 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 2) != 8 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 3) != 8 ||
					ifd.GetUint(TAG_BITS_PER_SAMPLE, 4) != 8)
					return B_NO_TRANSLATOR;
				
				imageType = TIFF_CMYK;
				break;
					
			default:
				return B_NO_TRANSLATOR;
		}
		
		uint32 rowsPerStrip, stripsPerImage;
		if (!ifd.HasField(TAG_ROWS_PER_STRIP))
			rowsPerStrip = DEFAULT_ROWS_PER_STRIP;
		else
			rowsPerStrip = ifd.GetUint(TAG_ROWS_PER_STRIP);
		if (rowsPerStrip == DEFAULT_ROWS_PER_STRIP)
			stripsPerImage = 1;
		else
			stripsPerImage = (height + rowsPerStrip - 1) / rowsPerStrip;
			
		if (ifd.GetCount(TAG_STRIP_OFFSETS) != stripsPerImage ||
			ifd.GetCount(TAG_STRIP_BYTE_COUNTS) != stripsPerImage)
			return B_NO_TRANSLATOR;
		
		// return read in details
		pdetails->width				= width;
		pdetails->height			= height;
		pdetails->compression		= compression;
		pdetails->t4options			= t4options;
		pdetails->rowsPerStrip		= rowsPerStrip;
		pdetails->stripsPerImage	= stripsPerImage;
		pdetails->interpretation	= interpretation;
		pdetails->bitsPerPixel		= bitsPerPixel;
		pdetails->imageType			= imageType;
		pdetails->fillOrder			= fillOrder;
			
		if (bAllocArrays) {
			ifd.GetUint32Array(TAG_STRIP_OFFSETS,
				&pdetails->pstripOffsets);
			ifd.GetUint32Array(TAG_STRIP_BYTE_COUNTS,
				&pdetails->pstripByteCounts);
				
			if (pdetails->imageType == TIFF_PALETTE)
				ifd.GetAdjustedColorMap(&pdetails->pcolorMap);
		}
			
	} catch (TiffIfdException) {
		return B_NO_TRANSLATOR;
	}
		
	return B_OK;
}

// copy descriptive information about the TIFF image
// into strname
void
copy_tiff_name(char *strname, TiffDetails *pdetails, swap_action swp)
{
	if (!strname || !pdetails)
		return;
		
	strcpy(strname, "TIFF image (");
	
	// byte order
	if (swp == B_SWAP_BENDIAN_TO_HOST)
		strcat(strname, "Big, ");
	else
		strcat(strname, "Little, ");
		
	// image type
	const char *strtype = NULL;
	switch (pdetails->imageType) {
		case TIFF_BILEVEL:
			if (pdetails->bitsPerPixel == 1)
				strtype = "Mono, ";
			else
				strtype = "Gray, ";
			break;
		case TIFF_PALETTE:
			strtype = "Palette, ";
			break;
		case TIFF_RGB:
			strtype = "RGB, ";
			break;
		case TIFF_CMYK:
			strtype = "CMYK, ";
			break;
		default:
			strtype = "??, ";
			break;
	}
	strcat(strname, strtype);
	
	// compression
	const char *strcomp = NULL;
	switch (pdetails->compression) {
		case COMPRESSION_NONE:
			strcomp = "None)";
			break;
		case COMPRESSION_HUFFMAN:
			strcomp = "Huffman)";
			break;
		case COMPRESSION_T4:
			strcomp = "Group 3)";
			break;
		case COMPRESSION_T6:
			strcomp = "Group 4)";
			break;
		case COMPRESSION_LZW:
			strcomp = "LZW)";
			break;
		case COMPRESSION_PACKBITS:
			strcomp = "PackBits)";
			break;	
		default:
			strcomp = "??)";
			break;
	}
	strcat(strname, strcomp);
}

status_t
identify_tiff_header(BPositionIO *inSource, BMessage *ioExtension,
	translator_info *outInfo, ssize_t amtread, uint8 *read, swap_action swp,
	TiffDetails *pdetails = NULL)
{
	if (amtread != 4)
		return B_ERROR;
		
	uint32 firstIFDOffset = 0;
	ssize_t nread = inSource->Read(&firstIFDOffset, 4);
	if (nread != 4) {
		// Unable to read first IFD offset
		return B_NO_TRANSLATOR;
	}
	if (swap_data(B_UINT32_TYPE, &firstIFDOffset,
		sizeof(uint32), swp) != B_OK) {
		// swap_data() error
		return B_ERROR;
	}
	TiffIfdList ifdlist(firstIFDOffset, *inSource, swp);
	
	status_t initcheck;
	initcheck = ifdlist.InitCheck();
		
	// Read in some fields in order to determine whether or not
	// this particular TIFF image is supported by this translator
	
	// Check the required fields
	if (initcheck == B_OK) {
	
		int32 document_count, document_index = 1;
		document_count = ifdlist.GetCount();
		
		if (ioExtension) {
			// Add page count to ioExtension
			ioExtension->RemoveName(DOCUMENT_COUNT);
			ioExtension->AddInt32(DOCUMENT_COUNT, document_count);
			
			// Check if a document index has been specified	
			ioExtension->FindInt32(DOCUMENT_INDEX, &document_index);
			if (document_index < 1 || document_index > document_count)
				return B_NO_TRANSLATOR;
		}
			
		// Identify the page the user asked for
		TiffIfd *pifd = ifdlist.GetIfd(document_index - 1);
			
		bool bAllocArrays = true;
		TiffDetails localdetails;
		if (!pdetails) {
			bAllocArrays = false;
			pdetails = &localdetails;
		}
		status_t result;
		result = check_tiff_fields(*pifd, pdetails, bAllocArrays);
		
		if (result == B_OK && outInfo) {
			outInfo->type = B_TIFF_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = TIFF_IN_QUALITY;
			outInfo->capability = TIFF_IN_CAPABILITY;
			strcpy(outInfo->MIME, "image/tiff");
			
			copy_tiff_name(outInfo->name, pdetails, swp);
		}
		
		return result;
	}
		
	if (initcheck != B_ERROR && initcheck != B_NO_MEMORY)
		// return B_NO_TRANSLATOR if unexpected data was encountered
		return B_NO_TRANSLATOR;
	else
		// return B_ERROR or B_NO_MEMORY
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
				// Byte Order: little endian
		} else if (memcmp(ch, kbeSig, 4) == 0) {
			swp = B_SWAP_BENDIAN_TO_HOST;
				// Byte Order: big endian
		} else {
			// If not a TIFF or a Be Bitmap image
			// (invalid byte order value)
			return B_NO_TRANSLATOR;
		}
		
		return identify_tiff_header(inSource, ioExtension, outInfo, 4, ch, swp);
	}
}

void
tiff_to_bits(uint8 *ptiff, uint32 tifflen, uint8 *pbits, TiffDetails &details)
{
	uint8 *ptiffend = ptiff + tifflen;
	
	switch (details.imageType) {
		case TIFF_BILEVEL:
			if (details.bitsPerPixel == 1) {
				// black and white
				uint8 black[3] = { 0x00, 0x00, 0x00 },
					white[3] = { 0xFF, 0xFF, 0xFF };
				uint8 *colors[2];
				if (details.interpretation == PHOTO_WHITEZERO) {
					colors[0] = white;
					colors[1] = black;
				} else {
					colors[0] = black;
					colors[1] = white;
				}
			
				for (uint32 i = 0; i < details.width; i++) {
					uint8 eightbits = (ptiff + (i / 8))[0];
					uint8 abit;
					abit = (eightbits >> (7 - (i % 8))) & 1;
				
					memcpy(pbits + (i * 4), colors[abit], 3);
				}
			} else {
				// grayscale
				if (details.bitsPerPixel == 8) {
					if (details.interpretation == PHOTO_WHITEZERO) {
						for (uint32 i = 0; i < details.width; i++) {
							memset(pbits, 255 - ptiff[i], 3);
							pbits += 4;
						}
					} else {
						for (uint32 i = 0; i < details.width; i++) {
							memset(pbits, ptiff[i], 3);
							pbits += 4;
						}
					}
				} else if (details.bitsPerPixel == 4) {
					const uint8 mask = 0x0f;
					if (details.interpretation == PHOTO_WHITEZERO) {
						for (uint32 i = 0; i < details.width; i++) {
							uint8 values = (ptiff + (i / 2))[0];
							uint8 value = (values >> (4 * (1 - (i % 2)))) & mask;
							memset(pbits, 255 - (value * 16), 3);
							pbits += 4;
						}
					} else {
						for (uint32 i = 0; i < details.width; i++) {
							uint8 values = (ptiff + (i / 2))[0];
							uint8 value = (values >> (4 * (1 - (i % 2)))) & mask;
							memset(pbits, (value * 16), 3);
							pbits += 4;
						}
					}
				}
			}
			
			break;
			
		case TIFF_PALETTE:
		{
			// NOTE: All of the entries in the color map were
			// divided by 256 before they were copied to pcolorMap.
			//
			// The first 3rd of the color map is made of the Red values,
			// the second 3rd of the color map is made of the Green values,
			// and the last 3rd of the color map is made of the Blue values.
			if (details.bitsPerPixel == 8) {
				for (uint32 i = 0; i < details.width; i++) {
					pbits[2] = details.pcolorMap[ptiff[i]];
					pbits[1] = details.pcolorMap[256 + ptiff[i]];
					pbits[0] = details.pcolorMap[512 + ptiff[i]];

					pbits += 4;
				}
			} else if (details.bitsPerPixel == 4) {
				const uint8 mask = 0x0f;
				for (uint32 i = 0; i < details.width; i++) {
					uint8 indices = (ptiff + (i / 2))[0];
					uint8 index = (indices >> (4 * (1 - (i % 2)))) & mask;
					
					pbits[2] = details.pcolorMap[index];
					pbits[1] = details.pcolorMap[16 + index];
					pbits[0] = details.pcolorMap[32 + index];
					
					pbits += 4;
				}
			}
			break;
		}
			
		case TIFF_RGB:
			while (ptiff < ptiffend) {
				pbits[2] = ptiff[0];
				pbits[1] = ptiff[1];
				pbits[0] = ptiff[2];

				ptiff += 3;
				pbits += 4;
			}
			break;
			
		case TIFF_CMYK:
		{
			while (ptiff < ptiffend) {
				int32 comp;
				comp = 255 - ptiff[0] - ptiff[3];
				pbits[2] = (comp < 0) ? 0 : comp;

				comp = 255 - ptiff[1] - ptiff[3];
				pbits[1] = (comp < 0) ? 0 : comp;
				
				comp = 255 - ptiff[2] - ptiff[3];
				pbits[0] = (comp < 0) ? 0 : comp;
				
				ptiff += 4;
				pbits += 4;
			}
			break;
		}
	}
}

// unpack the Packbits compressed data from
// pstreambuf and put it into tiffbuffer
ssize_t
unpack(StreamBuffer *pstreambuf, uint8 *tiffbuffer, uint32 tiffbufferlen)
{
	uint32 tiffwrit = 0, read = 0, len;
	while (tiffwrit < tiffbufferlen) {
	
		uint8 uctrl;
		if (pstreambuf->Read(&uctrl, 1) != 1)
			return B_ERROR;
		read++;
			
		int32 control;
		control = static_cast<signed char>(uctrl);
				
		// copy control + 1 bytes literally
		if (control >= 0 && control <= 127) {
		
			len = control + 1;
			if (tiffwrit + len > tiffbufferlen)
				return B_ERROR;
			if (pstreambuf->Read(tiffbuffer + tiffwrit, len) !=
				static_cast<ssize_t>(len))
				return B_ERROR;
			read += len;
				
			tiffwrit += len;
		
		// copy the next byte (-control) + 1 times
		} else if (control >= -127 && control <= -1) {
		
			uint8 byt;
			if (pstreambuf->Read(&byt, 1) != 1)
				return B_ERROR;
			read++;
				
			len = (-control) + 1;
			if (tiffwrit + len > tiffbufferlen)
				return B_ERROR;
			memset(tiffbuffer + tiffwrit, byt, len);
			
			tiffwrit += len;			
		}
	}
	
	return read;
}

ssize_t
TIFFTranslator::decode_t4(BitReader &stream, TiffDetails &details, 
	uint8 *pbits, bool bfirstLine)
{
	if (bfirstLine) {
		// determine number of fill bits
		// (if that T4Option is set)
		uint32 fillbits = 0;
		if (details.t4options & T4_FILL_BYTE_BOUNDARY) {
			fillbits = (12 - stream.BitsInBuffer()) % 8;
			if (fillbits)
				fillbits = 8 - fillbits;
		}
		// read in and verify end-of-line sequence
		uint32 nzeros = 11 + fillbits;
		while (nzeros--) {
			if (stream.NextBit() != 0) {
				debugger("EOL: expected zero bit");
				return B_ERROR;
			}
		}
		if (stream.NextBit() != 1) {
			debugger("EOL: expected one bit");
			return B_ERROR;
		}
	}
		
	const uint8 kblack = 0x00, kwhite = 0xff;
	uint8 colors[2];
	if (details.interpretation == PHOTO_WHITEZERO) {
		colors[0] = kwhite;
		colors[1] = kblack;
	} else {
		colors[0] = kblack;
		colors[1] = kwhite;
	}

	uint32 pixelswrit = 0;	
	DecodeTree *ptrees[2] = {fpwhiteTree, fpblackTree};
	uint8 currentcolor = 0;
	uint8 *pcurrentpixel = pbits;
	status_t value = 0;
	while (pixelswrit < details.width || value >= 64) {
		value = ptrees[currentcolor]->GetValue(stream);
		if (value < 0) {
			debugger("value < 0");
			return B_ERROR;
		}
			
		if (pixelswrit + value > details.width) {
			debugger("gone past end of line");
			return B_ERROR;
		}
		pixelswrit += value;
		
		// covert run length to B_RGB32 pixels
		uint32 pixelsleft = value;
		while (pixelsleft--) {
			memset(pcurrentpixel, colors[currentcolor], 3);
			pcurrentpixel += 4;
		}
		
		if (value < 64)
			currentcolor = 1 - currentcolor;
	}
	
	// determine number of fill bits
	// (if that T4Option is set)
	uint32 fillbits = 0;
	if (details.t4options & T4_FILL_BYTE_BOUNDARY) {
		fillbits = (12 - stream.BitsInBuffer()) % 8;
		if (fillbits)
			fillbits = 8 - fillbits;
	}
	// read in end-of-line sequence
	uint32 ndontcare = 12 + fillbits;
	status_t sbit = 0;
	while (ndontcare--) {
		sbit = stream.NextBit();
		if (sbit != 0 && sbit != 1) {
			debugger("EOL: couldn't read all bits");
			return B_ERROR;
		}
	}
	
	return stream.BytesRead();
}

ssize_t
TIFFTranslator::decode_huffman(StreamBuffer *pstreambuf, TiffDetails &details, 
	uint8 *pbits)
{
	BitReader stream(details.fillOrder, pstreambuf);
	if (stream.InitCheck() != B_OK) {
		debugger("stream init error");
		return B_ERROR;
	}
		
	const uint8 kblack = 0x00, kwhite = 0xff;
	uint8 colors[2];
	if (details.interpretation == PHOTO_WHITEZERO) {
		colors[0] = kwhite;
		colors[1] = kblack;
	} else {
		colors[0] = kblack;
		colors[1] = kwhite;
	}

	uint32 pixelswrit = 0;	
	DecodeTree *ptrees[2] = {fpwhiteTree, fpblackTree};
	uint8 currentcolor = 0;
	uint8 *pcurrentpixel = pbits;
	status_t value = 0;
	while (pixelswrit < details.width || value >= 64) {
		value = ptrees[currentcolor]->GetValue(stream);
		if (value < 0) {
			debugger("value < 0");
			return B_ERROR;
		}
			
		if (pixelswrit + value > details.width) {
			debugger("gone past end of line");
			return B_ERROR;
		}
		pixelswrit += value;
		
		// covert run length to B_RGB32 pixels
		uint32 pixelsleft = value;
		while (pixelsleft--) {
			memset(pcurrentpixel, colors[currentcolor], 3);
			pcurrentpixel += 4;
		}
		
		if (value < 64)
			currentcolor = 1 - currentcolor;
	}
	
	return stream.BytesRead();
}

status_t
TIFFTranslator::translate_from_tiff(BPositionIO *inSource,
	BMessage *ioExtension, ssize_t amtread, uint8 *read, swap_action swp,
	uint32 outType,	BPositionIO *outDestination)
{
	// Can only output to bits for now
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	status_t result;
	
	TiffDetails details;
	result = identify_tiff_header(inSource, ioExtension, NULL,
		amtread, read, swp, &details);
	if (result == B_OK) {
		// If the TIFF is supported by this translator
		
		// If TIFF uses Huffman compression, load
		// trees for decoding Huffman compression
		if (details.compression == COMPRESSION_HUFFMAN ||
			details.compression == COMPRESSION_T4) {
			result = LoadHuffmanTrees();
			if (result != B_OK)
				return result;
		}
		
		TranslatorBitmap bitsHeader;
		bitsHeader.magic = B_TRANSLATOR_BITMAP;
		bitsHeader.bounds.left = 0;
		bitsHeader.bounds.top = 0;
		bitsHeader.bounds.right = details.width - 1;
		bitsHeader.bounds.bottom = details.height - 1;
		bitsHeader.rowBytes = 4 * details.width;
		bitsHeader.colors = B_RGB32;
		bitsHeader.dataSize = bitsHeader.rowBytes * details.height;
		
		// write out Be's Bitmap header
		if (swap_data(B_UINT32_TYPE, &bitsHeader,
			sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK)
			return B_ERROR;
		outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
		
		uint32 inbufferlen, outbufferlen;
		outbufferlen = 4 * details.width;
		switch (details.imageType) {
			case TIFF_BILEVEL:
			case TIFF_PALETTE:
			{
				uint32 pixelsPerByte = 8 / details.bitsPerPixel;
				inbufferlen = (details.width / pixelsPerByte) +
					((details.width % pixelsPerByte) ? 1 : 0);
				break;
			}
			case TIFF_RGB:
				inbufferlen = 3 * details.width;
				break;
				
			case TIFF_CMYK:
				inbufferlen = 4 * details.width;
				break;
				
			default:
				// just to be safe
				return B_NO_TRANSLATOR;
		}
		
		uint8 *inbuffer = new uint8[inbufferlen],
			*outbuffer = new uint8[outbufferlen];
		if (!inbuffer || !outbuffer)
			return B_NO_MEMORY;
		// set all channels to 0xff so that I won't have
		// to set the alpha byte to 0xff every time
		memset(outbuffer, 0xff, outbufferlen);
		
		// buffer for making reading compressed data
		// fast and convenient
		StreamBuffer *pstreambuf = NULL;
		BitReader bitreader(details.fillOrder, pstreambuf, false);
			// this creates an invalid bitreader, but it is
			// initialized properly with the SetTo() call
		if (details.compression != COMPRESSION_NONE) {
			pstreambuf = new StreamBuffer(inSource, 2048, false);
			if (pstreambuf->InitCheck() != B_OK)
				return B_NO_MEMORY; 
		}
		
		bool bfirstLine = true;	
		for (uint32 i = 0; i < details.stripsPerImage; i++) {
			uint32 read = 0;
			
			// If using compression, prepare streambuffer
			// for reading
			if (details.compression != COMPRESSION_NONE &&
				!pstreambuf->Seek(details.pstripOffsets[i]))
				return B_NO_TRANSLATOR;
				
			if (details.compression == COMPRESSION_T4 &&
				bitreader.SetTo(details.fillOrder, pstreambuf) != B_OK)
				return B_NO_TRANSLATOR;
				
			// Read / Write one line at a time for each
			// line in the strip
			while (read < details.pstripByteCounts[i]) {
			
				ssize_t ret = 0;
				switch (details.compression) {
					case COMPRESSION_NONE:
						ret = inSource->ReadAt(details.pstripOffsets[i] + read,
							inbuffer, inbufferlen);
						if (ret != static_cast<ssize_t>(inbufferlen))
							// break out of while loop
							ret = -1;
						break;
							
					case COMPRESSION_HUFFMAN:
						ret = decode_huffman(pstreambuf, details, outbuffer);
						if (ret < 1)
							// break out of while loop
							ret = -1;
						break;
						
					case COMPRESSION_T4:
						ret = decode_t4(bitreader, details, outbuffer,
							bfirstLine);
						if (ret < 1)
							// break out of while loop
							ret = -1;
						else
							ret -= read;
								// unlike other decoders, decode_t4 retruns
								// the total bytes read from the current
								// strip
						break;
						
					case COMPRESSION_PACKBITS:
						ret = unpack(pstreambuf, inbuffer, inbufferlen);
						if (ret < 1)
							// break out of while loop
							ret = -1;
						break;
				}
				if (ret < 0)
					break;
					
				read += ret;
				if (details.compression != COMPRESSION_HUFFMAN &&
					details.compression != COMPRESSION_T4)
					tiff_to_bits(inbuffer, inbufferlen, outbuffer, details);
				outDestination->Write(outbuffer, outbufferlen);
				bfirstLine = false;
			}
			// If while loop was broken...
			if (read < details.pstripByteCounts[i]) {
				debugger("while loop broken");
				break;
			}
		}
		
		// Clean up
		if (pstreambuf) {
			delete pstreambuf;
			pstreambuf = NULL;
		}
		
		delete[] inbuffer;
		inbuffer = NULL;
		delete[] outbuffer;
		outbuffer = NULL;
		
		return B_OK;
		
	} else
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
TIFFTranslator::Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination)
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
		return B_NO_TRANSLATOR;
			// Not implemented yet
		
	// Might be TIFF image
	else {
		// TIFF Byte Order / Magic
		const uint8 kleSig[] = { 0x49, 0x49, 0x2a, 0x00 };
		const uint8 kbeSig[] = { 0x4d, 0x4d, 0x00, 0x2a };
		
		swap_action swp;
		if (memcmp(ch, kleSig, 4) == 0) {
			swp = B_SWAP_LENDIAN_TO_HOST;
				// Byte Order: little endian
		} else if (memcmp(ch, kbeSig, 4) == 0) {
			swp = B_SWAP_BENDIAN_TO_HOST;
				// Byte Order: big endian
		} else {
			// If not a TIFF or a Be Bitmap image
			// (invalid byte order value)
			return B_NO_TRANSLATOR;
		}
		
		return translate_from_tiff(inSource, ioExtension, 4, ch, swp, outType,
			outDestination);
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

// Initialize the Huffman decoding trees and
// verify that there were no initialization errors
status_t
TIFFTranslator::LoadHuffmanTrees()
{
	if (!fpblackTree) {
		fpblackTree = new DecodeTree(false);
		if (!fpblackTree)
			return B_NO_MEMORY;
	}
	if (!fpwhiteTree) {
		fpwhiteTree = new DecodeTree(true);
		if (!fpwhiteTree)
			return B_NO_MEMORY;
	}
	
	if (fpblackTree->InitCheck() != B_OK)
		return fpblackTree->InitCheck();
	
	return fpwhiteTree->InitCheck();
}

