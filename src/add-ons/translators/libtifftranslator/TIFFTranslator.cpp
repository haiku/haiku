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
#include "tiffio.h"
#include "TIFFTranslator.h"
#include "TIFFView.h"

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


//// libtiff Callback functions!

BPositionIO *
tiff_get_pio(thandle_t stream)
{
	BPositionIO *pio = NULL;
	pio = static_cast<BPositionIO *>(stream);
	if (!pio)
		debugger("pio is NULL");
		
	return pio;
}

tsize_t
tiff_read_proc(thandle_t stream, tdata_t buf, tsize_t size)
{
	return tiff_get_pio(stream)->Read(buf, size);
}

tsize_t
tiff_write_proc(thandle_t stream, tdata_t buf, tsize_t size)
{
	return tiff_get_pio(stream)->Write(buf, size);
}

toff_t
tiff_seek_proc(thandle_t stream, toff_t off, int whence)
{
	return tiff_get_pio(stream)->Seek(off, whence);
}

int
tiff_close_proc(thandle_t stream)
{
	tiff_get_pio(stream)->Seek(0, SEEK_SET);
	return 0;
}

toff_t
tiff_size_proc(thandle_t stream)
{
	BPositionIO *pio = tiff_get_pio(stream);
	off_t cur, end;
	cur = pio->Position();
	end = pio->Seek(0, SEEK_END);
	pio->Seek(cur, SEEK_SET);
	
	return end;
}

int
tiff_map_file_proc(thandle_t stream, tdata_t *pbase, toff_t *psize)
{
	// BeOS doesn't support mmap() so just return 0
	return 0;
}

void
tiff_unmap_file_proc(thandle_t stream, tdata_t base, toff_t size)
{
	return;
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
		static_cast<int>(TIFF_TRANSLATOR_VERSION >> 8),
		static_cast<int>((TIFF_TRANSLATOR_VERSION >> 4) & 0xf),
		static_cast<int>(TIFF_TRANSLATOR_VERSION & 0xf), __DATE__);
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

status_t
identify_tiff_header(BPositionIO *inSource, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType, TIFF **poutTIFF = NULL)
{
	// Can only output to bits for now
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;
	
	// reset pos to start
	inSource->Seek(0, SEEK_SET);
	
	// get TIFF handle	
	TIFF* tif = TIFFClientOpen("TIFFTranslator", "r", inSource,
		tiff_read_proc, tiff_write_proc, tiff_seek_proc, tiff_close_proc,
		tiff_size_proc, tiff_map_file_proc, tiff_unmap_file_proc); 
    if (!tif)
    	return B_NO_TRANSLATOR;
	
	// count number of documents
	int32 document_count = 0, document_index = 1;
	do {
		document_count++;
	} while (TIFFReadDirectory(tif));
	
	if (ioExtension) {
		// add page count to ioExtension
		ioExtension->RemoveName(DOCUMENT_COUNT);
		ioExtension->AddInt32(DOCUMENT_COUNT, document_count);
		
		// Check if a document index has been specified
		status_t fnd = ioExtension->FindInt32(DOCUMENT_INDEX, &document_index);
		if (fnd == B_OK && (document_index < 1 || document_index > document_count))
			// If a document index has been supplied, and it is an invalid value,
			// return failure
			return B_NO_TRANSLATOR;
		else if (fnd != B_OK)
			// If FindInt32 failed, make certain the document index
			// is the default value
			document_index = 1;
	}
	
	// identify the document the user specified or the first document
	// if the user did not specify which document they wanted to identify
	if (!TIFFSetDirectory(tif, document_index - 1))
		return B_NO_TRANSLATOR;

	if (outInfo) {
		outInfo->type = B_TIFF_FORMAT;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = TIFF_IN_QUALITY;
		outInfo->capability = TIFF_IN_CAPABILITY;
		strcpy(outInfo->MIME, "image/tiff");
		sprintf(outInfo->name, "TIFF image (page %d of %d)",
			static_cast<int>(document_index), static_cast<int>(document_count));
	}
	
	if (!poutTIFF)
		// close TIFF if caller is not interested in TIFF handle
		TIFFClose(tif);
	else
		// leave TIFF open and return handle if caller needs it
		*poutTIFF = tif;
		
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
	bheaderonly = bdataonly = false;
		// only allow writing of the entire image
		// (fix for buggy programs that lie about what they actually need)
	
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
		
		if (memcmp(ch, kleSig, 4) != 0 && memcmp(ch, kbeSig, 4) != 0)
			// If not a TIFF or a Be Bitmap image
			// (invalid byte order value)
			return B_NO_TRANSLATOR;
		
		return identify_tiff_header(inSource, ioExtension, outInfo, outType);
	}
}

status_t
translate_from_tiff(BPositionIO *inSource, BMessage *ioExtension,
	uint32 outType, BPositionIO *outDestination, bool bheaderonly,
	bool bdataonly)
{
	status_t result = B_NO_TRANSLATOR;
	
	// variables needing cleanup
	TIFF *ptif = NULL;
	uint32 *praster = NULL;
	
	status_t ret;
	ret = identify_tiff_header(inSource, ioExtension, NULL, outType, &ptif);
	while (ret == B_OK && ptif) {
		// use while / break not for looping, but for 
		// cleaner goto like capability
		
		ret = B_ERROR;
			// make certain there is no looping
			
		uint32 width = 0, height = 0;
		if (!TIFFGetField(ptif, TIFFTAG_IMAGEWIDTH, &width)) {
			result = B_NO_TRANSLATOR;
			break;
		}
		if (!TIFFGetField(ptif, TIFFTAG_IMAGELENGTH, &height)) {
			result = B_NO_TRANSLATOR;
			break;
		}
		size_t npixels = 0;
		npixels = width * height;
		praster = static_cast<uint32 *>(_TIFFmalloc(npixels * 4));
		if (praster && TIFFReadRGBAImage(ptif, width, height, praster, 0)) {
			if (!bdataonly) {
				// Construct and write Be bitmap header
				TranslatorBitmap bitsHeader;
				bitsHeader.magic = B_TRANSLATOR_BITMAP;
				bitsHeader.bounds.left = 0;
				bitsHeader.bounds.top = 0;
				bitsHeader.bounds.right = width - 1;
				bitsHeader.bounds.bottom = height - 1;
				bitsHeader.rowBytes = 4 * width;
				bitsHeader.colors = B_RGBA32;
				bitsHeader.dataSize = bitsHeader.rowBytes * height;
				if (swap_data(B_UINT32_TYPE, &bitsHeader,
					sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
					result = B_ERROR;
					break;
				}
				outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
			}
			
			if (!bheaderonly) {
				// Convert raw RGBA data to B_RGBA32 colorspace
				// and write out the results
				uint8 *pbitsrow = new uint8[width * 4];
				if (!pbitsrow) {
					result = B_NO_MEMORY;
					break;
				}
				uint8 *pras8 = reinterpret_cast<uint8 *>(praster);
				for (uint32 i = 0; i < height; i++) {
					uint8 *pbits, *prgba;
					pbits = pbitsrow;
					prgba = pras8 + ((height - (i + 1)) * width * 4);
					
					for (uint32 k = 0; k < width; k++) {
						pbits[0] = prgba[2];
						pbits[1] = prgba[1];
						pbits[2] = prgba[0];
						pbits[3] = prgba[3];
						pbits += 4;
						prgba += 4;
					}

					outDestination->Write(pbitsrow, width * 4);
				}
				delete[] pbitsrow;
				pbitsrow = NULL;
			}

			result = B_OK;
			break;

		} // if (praster && TIFFReadRGBAImage(ptif, width, height, praster, 0))
		
	} // while (ret == B_OK && ptif)

	if (praster) {
		_TIFFfree(praster);
		praster = NULL;
	}
	if (ptif) {
		TIFFClose(ptif);
		ptif = NULL;
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
	bheaderonly = bdataonly = false;
		// only allow writing of the entire image
		// (fix for buggy programs that lie about what they actually need)
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	if (n32ch == nbits) {
		// B_TRANSLATOR_BITMAP type
		inSource->Seek(0, SEEK_SET);
		
		const size_t kbufsize = 2048;
		uint8 buffer[kbufsize];
		ssize_t ret = inSource->Read(buffer, kbufsize);
		while (ret > 0) {
			outDestination->Write(buffer, ret);
			ret = inSource->Read(buffer, kbufsize);
		}
		
		return B_OK;
		
	} else
		// Might be TIFF image
		return translate_from_tiff(inSource, ioExtension, outType,
			outDestination, bheaderonly, bdataonly);
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


