/*****************************************************************************/
// TGATranslator
// TGATranslator.cpp
//
// This BTranslator based object is for opening and writing TGA files.
//
//
// Copyright (c) 2002 OpenBeOS Project
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
#include "TGATranslator.h"
#include "TGAView.h"

#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x > y) ? x : y)

// The input formats that this translator supports.
translation_format gInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (TGATranslator)"
	},
	{
		B_TGA_FORMAT,
		B_TRANSLATOR_BITMAP,
		TGA_IN_QUALITY,
		TGA_IN_CAPABILITY,
		"image/x-targa",
		"Targa image"
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
		"Be Bitmap Format (TGATranslator)"
	},
	{
		B_TGA_FORMAT,
		B_TRANSLATOR_BITMAP,
		TGA_OUT_QUALITY,
		TGA_OUT_CAPABILITY,
		"image/x-targa",
		"Targa image"
	}
};

// ---------------------------------------------------------------
// make_nth_translator
//
// Creates a TGATranslator object to be used by BTranslatorRoster
//
// Preconditions:
//
// Parameters: n,		The translator to return. Since
//						TGATranslator only publishes one
//						translator, it only returns a
//						TGATranslator if n == 0
//
//             you, 	The image_id of the add-on that
//						contains code (not used).
//
//             flags,	Has no meaning yet, should be 0.
//
// Postconditions:
//
// Returns: NULL if n is not zero,
//          a new TGATranslator if n is zero
// ---------------------------------------------------------------
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new TGATranslator();
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
TGATranslator::TGATranslator()
	:	BTranslator()
{
	strcpy(fName, "TGA Images");
	sprintf(fInfo, "TGA image translator v%d.%d.%d %s\n",
		TGA_TRANSLATOR_VERSION / 100, (TGA_TRANSLATOR_VERSION / 10) % 10,
		TGA_TRANSLATOR_VERSION % 10, __DATE__);
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
TGATranslator::~TGATranslator()
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
TGATranslator::TranslatorName() const
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
TGATranslator::TranslatorInfo() const
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
TGATranslator::TranslatorVersion() const
{
	return TGA_TRANSLATOR_VERSION;
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
// formats through the out_count parameter
// ---------------------------------------------------------------
const translation_format *
TGATranslator::InputFormats(int32 *out_count) const
{
	if (out_count)
		*out_count = 2;

	return gInputFormats;
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
// formats through the out_count parameter
// ---------------------------------------------------------------	
const translation_format *
TGATranslator::OutputFormats(int32 *out_count) const
{
	if (out_count)
		*out_count = 2;

	return gOutputFormats;
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
	if (inSource->Read(((uint8 *) &header) + amtread, size) != size)
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

// ---------------------------------------------------------------
// identify_tga_header
//
// Determines if the data in inSource is in the MS or OS/2 TGA
// format. If it is, it returns info about the data in inSource
// to outInfo, pfileheader, pmsheader, pfrommsformat and os2skip.
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
//				pfileheader,	File header info for the TGA is
//								copied here after it is read from
//								the file.
//
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data does not look like
//								TGA format data
//
// B_ERROR,	if the header data could not be converted to host
//			format
//
// B_OK,	if the data looks like bits data and no errors were
//			encountered
// ---------------------------------------------------------------
status_t
identify_tga_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, uint8 *read, TGAFileHeader *pfileheader = NULL,
	TGAColorMapSpec *pmapspec = NULL, TGAImageSpec *pimagespec = NULL)
{
	uint8 buf[TGA_HEADERS_SIZE];
	memcpy(buf, read, amtread);
		// copy portion of TGA headers already read in
	// read in the rest of the TGA headers
	ssize_t size = TGA_HEADERS_SIZE - amtread;
	if (size > 0 && inSource->Read(buf + amtread, size) != size)
		return B_NO_TRANSLATOR;
	
	// Read in TGA file header
	TGAFileHeader fileheader;
	memcpy(&fileheader.idlength, buf, 1);
	
	memcpy(&fileheader.colormaptype, buf + 1, 1);
	if (fileheader.colormaptype > 1)
		return B_NO_TRANSLATOR;
		
	memcpy(&fileheader.imagetype, buf + 2, 1);
	if ((fileheader.imagetype > 3 && fileheader.imagetype < 9) ||
		fileheader.imagetype > 11)
		return B_NO_TRANSLATOR;
	if ((fileheader.colormaptype == TGA_NO_COLORMAP &&
		fileheader.imagetype == TGA_NOCOMP_COLORMAP) ||
		(fileheader.colormaptype == TGA_COLORMAP &&
			fileheader.imagetype != TGA_NOCOMP_COLORMAP &&
			fileheader.imagetype != TGA_RLE_COLORMAP))
		return B_NO_TRANSLATOR;
	
	// Read in TGA color map spec
	TGAColorMapSpec mapspec;
	memcpy(&mapspec.firstentry, buf + 3, 2);
	mapspec.firstentry = B_LENDIAN_TO_HOST_INT16(mapspec.firstentry);
	if (fileheader.colormaptype == 0 && mapspec.firstentry != 0)
		return B_NO_TRANSLATOR;
	
	memcpy(&mapspec.length, buf + 5, 2);
	mapspec.length = B_LENDIAN_TO_HOST_INT16(mapspec.length);
	if (fileheader.colormaptype == TGA_NO_COLORMAP &&
		mapspec.length != 0)
		return B_NO_TRANSLATOR;
	if (fileheader.colormaptype == TGA_COLORMAP &&
		mapspec.length == 0)
		return B_NO_TRANSLATOR;
	
	memcpy(&mapspec.entrysize, buf + 7, 1);
	if (fileheader.colormaptype == TGA_NO_COLORMAP &&
		mapspec.entrysize != 0)
		return B_NO_TRANSLATOR;
	if (fileheader.colormaptype == TGA_COLORMAP &&
		mapspec.entrysize != 15 && mapspec.entrysize != 16 &&
		mapspec.entrysize != 24 && mapspec.entrysize != 32)
		return B_NO_TRANSLATOR;
	
	// Read in TGA image spec
	TGAImageSpec imagespec;
	memcpy(&imagespec.xorigin, buf + 8, 2);
	imagespec.xorigin = B_LENDIAN_TO_HOST_INT16(imagespec.xorigin);
	
	memcpy(&imagespec.yorigin, buf + 10, 2);
	imagespec.yorigin = B_LENDIAN_TO_HOST_INT16(imagespec.yorigin);
	
	memcpy(&imagespec.width, buf + 12, 2);
	imagespec.width = B_LENDIAN_TO_HOST_INT16(imagespec.width);
	if (imagespec.width == 0)
		return B_NO_TRANSLATOR;
	
	memcpy(&imagespec.height, buf + 14, 2);
	imagespec.height = B_LENDIAN_TO_HOST_INT16(imagespec.height);
	if (imagespec.height == 0)
		return B_NO_TRANSLATOR;
	
	memcpy(&imagespec.depth, buf + 16, 1);
	if (imagespec.depth < 1 || imagespec.depth > 32)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
			fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth != 15 && imagespec.depth != 16 &&
		imagespec.depth != 24 && imagespec.depth != 32)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_BW ||
			fileheader.imagetype == TGA_RLE_BW) &&
		imagespec.depth != 8)
		return B_NO_TRANSLATOR;
	if (fileheader.colormaptype == TGA_COLORMAP &&
		imagespec.depth != 8)
		return B_NO_TRANSLATOR;
	
	memcpy(&imagespec.descriptor, buf + 17, 1);
	// images ordered from Right to Left (rather than Left to Right)
	// are not supported
	if (imagespec.descriptor & TGA_ORIGIN_HORZ_BIT == TGA_ORIGIN_RIGHT)
		return B_NO_TRANSLATOR;
	// unused descriptor bits, these bits must be zero
	if (imagespec.descriptor & TGA_DESC_BITS76)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 32 &&
		imagespec.descriptor & TGA_DESC_ALPHABITS != 8 &&
		imagespec.descriptor & TGA_DESC_ALPHABITS != 0)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 24 &&
		imagespec.descriptor & TGA_DESC_ALPHABITS != 0)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 16 &&
		imagespec.descriptor & TGA_DESC_ALPHABITS != 1 &&
		imagespec.descriptor & TGA_DESC_ALPHABITS != 0)
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 15 &&
		imagespec.descriptor & TGA_DESC_ALPHABITS != 0)
		return B_NO_TRANSLATOR;
		
	// Fill in headers passed to this function
	if (pfileheader) {
		pfileheader->idlength = fileheader.idlength;
		pfileheader->colormaptype = fileheader.colormaptype;
		pfileheader->imagetype = fileheader.imagetype;
	}
	if (pmapspec) {
		pmapspec->firstentry = mapspec.firstentry;
		pmapspec->length = mapspec.length;
		pmapspec->entrysize = mapspec.entrysize;
	}
	if (pimagespec) {
		pimagespec->xorigin = imagespec.xorigin;
		pimagespec->yorigin = imagespec.yorigin;
		pimagespec->width = imagespec.width;
		pimagespec->height = imagespec.height;
		pimagespec->depth = imagespec.depth;
		pimagespec->descriptor = imagespec.descriptor;
	}
	
	if (outInfo) {
		outInfo->type = B_TGA_FORMAT;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = TGA_IN_QUALITY;
		outInfo->capability = TGA_IN_CAPABILITY;
		sprintf(outInfo->name, "Targa image (%d bits",
			imagespec.depth);
		switch (fileheader.imagetype) {
			case TGA_NOCOMP_COLORMAP:
				strcat(outInfo->name, " colormap");
				break;
			case TGA_NOCOMP_TRUECOLOR:
				strcat(outInfo->name, " truecolor");
				break;
			case TGA_NOCOMP_BW:
				strcat(outInfo->name, " gray");
				break;
			case TGA_RLE_COLORMAP:
				strcat(outInfo->name, " RLE colormap");
				break;
			case TGA_RLE_TRUECOLOR:
				strcat(outInfo->name, " RLE truecolor");
				break;
			case TGA_RLE_BW:
				strcat(outInfo->name, " RLE gray");
				break;
		}
		strcat(outInfo->name, ")");
		strcpy(outInfo->MIME, "image/x-targa");
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
//								translator
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
// B_OK,	if this translator understand the data and there were
//			no errors found
// ---------------------------------------------------------------
status_t
TGATranslator::Identify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_TGA_FORMAT)
		return B_NO_TRANSLATOR;

	uint8 ch[4];
	uint32 nbits = B_TRANSLATOR_BITMAP;
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	if (swap_data(B_UINT32_TYPE, &nbits, 4, B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
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
	// if NOT B_TRANSLATOR_BITMAP, it could be
	// an image in the TGA format
	else
		return identify_tga_header(inSource, outInfo, 4, ch);
}

// ---------------------------------------------------------------
// translate_from_bits_to_tgatc
//
// Converts various varieties of the Be Bitmap format ('bits') to
// the TGA True Color format
//
// Preconditions:
//
// Parameters:	inSource,	contains the bits data to convert
//
//				outDestination,	where the TGA data will be written
//
//				fromspace,	the format of the data in inSource
//
//
// Postconditions:
//
// Returns: B_ERROR,	if memory couldn't be allocated or another
//						error occured
//
// B_OK,	if no errors occurred
// ---------------------------------------------------------------
status_t
translate_from_bits_to_tgatc(BPositionIO *inSource,
BPositionIO *outDestination, color_space fromspace, TGAImageSpec &imagespec)
{
	int32 bitsBytesPerPixel = 0;
	switch (fromspace) {
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_CMY32:
		case B_CMYA32:
		case B_CMYK32:
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
			
		case B_CMAP8:
		case B_GRAY8:
			bitsBytesPerPixel = 1;
			break;
			
		default:
			return B_ERROR;
	}
	int32 bitsRowBytes = imagespec.width * bitsBytesPerPixel;
	int32 tgaBytesPerPixel = imagespec.depth / 8;
	if (imagespec.depth % 8)
		tgaBytesPerPixel++;

	int32 tgaRowBytes = (imagespec.width * tgaBytesPerPixel);
	uint32 tgapixrow = 0;
	uint8 *tgaRowData = new uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		return B_ERROR;
	}
	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	const color_map *pmap = system_colors();
	while (rd == bitsRowBytes) {
	
		for (uint32 i = 0; i < imagespec.width; i++) {
			uint8 *bitspixel, *tgapixel;
			uint16 val;
			int32 comp;
			switch (fromspace) {
				case B_RGBA32:
					memcpy(tgaRowData + (i * 4),
						bitsRowData + (i * bitsBytesPerPixel), 4);
					break;
					
				case B_RGBA32_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 4);
					tgapixel[0] = bitspixel[3];
					tgapixel[1] = bitspixel[2];
					tgapixel[2] = bitspixel[1];
					tgapixel[3] = bitspixel[0];
					break;
				
				case B_CMYA32:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 4);
					tgapixel[0] = 255 - bitspixel[2];
					tgapixel[1] = 255 - bitspixel[1];
					tgapixel[2] = 255 - bitspixel[0];
					tgapixel[3] = bitspixel[3];
					break;
					
				case B_RGB32:
				case B_RGB24:
					memcpy(tgaRowData + (i * 3),
						bitsRowData + (i * bitsBytesPerPixel), 3);
					break;
					
				case B_CMYK32:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					
					comp = 255 - bitspixel[2] - bitspixel[3];
					tgapixel[0] = (comp < 0) ? 0 : comp;
					
					comp = 255 - bitspixel[1] - bitspixel[3];
					tgapixel[1] = (comp < 0) ? 0 : comp;
					
					comp = 255 - bitspixel[0] - bitspixel[3];
					tgapixel[2] = (comp < 0) ? 0 : comp;
					break;
				
				case B_CMY32:
				case B_CMY24:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					tgapixel[0] = 255 - bitspixel[2];
					tgapixel[1] = 255 - bitspixel[1];
					tgapixel[2] = 255 - bitspixel[0];
					break;
					
				case B_RGB16:
				case B_RGB16_BIG:
					// Expand to 24 bit because the TGA format handles
					// 16 bit images differently than the Be Image Format
					// which would cause a loss in quality
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					if (fromspace == B_RGB16)
						val = bitspixel[0] + (bitspixel[1] << 8);
					else
						val = bitspixel[1] + (bitspixel[0] << 8);

					tgapixel[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					tgapixel[1] =
						((val & 0x7e0) >> 3) | ((val & 0x7e0) >> 9);
					tgapixel[2] =
						((val & 0xf800) >> 8) | ((val & 0xf800) >> 13);
					break;
				
				case B_RGBA15:
					memcpy(tgaRowData + (i * 2),
						bitsRowData + (i * bitsBytesPerPixel), 2);
					break;
				
				case B_RGBA15_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 2);
					tgapixel[0] = bitspixel[1];
					tgapixel[1] = bitspixel[0];
					break;

				case B_RGB15:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 2);
					tgapixel[0] = bitspixel[0];
					tgapixel[1] = bitspixel[1] | 0x80;
					break;
					
				case B_RGB15_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 2);
					tgapixel[0] = bitspixel[1];
					tgapixel[1] = bitspixel[0] | 0x80;
					break;
						
				case B_RGB32_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					tgapixel[0] = bitspixel[3];
					tgapixel[1] = bitspixel[2];
					tgapixel[2] = bitspixel[1];
					break;
						
				case B_RGB24_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					tgapixel[0] = bitspixel[2];
					tgapixel[1] = bitspixel[1];
					tgapixel[2] = bitspixel[0];
					break;
				
				case B_CMAP8:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					rgb_color c = pmap->color_list[bitspixel[0]];
					tgapixel[0] = c.blue;
					tgapixel[1] = c.green;
					tgapixel[2] = c.red;
					break;
					
				case B_GRAY8:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					tgapixel = tgaRowData + (i * 3);
					tgapixel[0] = bitspixel[0];
					tgapixel[1] = bitspixel[0];
					tgapixel[2] = bitspixel[0];
					break;
						
				default:
					break;
			} // switch (fromspace)
		} // for for (uint32 i = 0; i < imagespec.width; i++)
				
		outDestination->Write(tgaRowData, tgaRowBytes);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (tgapixrow == imagespec.height)
			break;

		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)
	
	delete[] tgaRowData;
	delete[] bitsRowData;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_bits8_to_tga8
//
// Converts 8-bit Be Bitmaps ('bits') to the 8-bit TGA format
//
// Preconditions:
//
// Parameters:	inSource,	contains the bits data to convert
//
//				outDestination,	where the TGA data will be written
//
//				bitsRowBytes,	number of bytes in one row of
//								bits data
//
// Postconditions:
//
// Returns: B_ERROR,	if memory couldn't be allocated or another
//						error occured
//
// B_OK,	if no errors occurred
// ---------------------------------------------------------------
status_t
translate_from_bits8_to_tga8(BPositionIO *inSource,
	BPositionIO *outDestination, int32 bitsRowBytes, uint16 height)
{
	uint32 tgapixrow = 0;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData)
		return B_ERROR;
	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	while (rd == bitsRowBytes) {
		outDestination->Write(bitsRowData, bitsRowBytes);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (tgapixrow == height)
			break;

		rd = inSource->Read(bitsRowData, bitsRowBytes);
	}
	
	delete[] bitsRowData;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_bits1_to_tgabw
//
// Converts 1-bit Be Bitmaps ('bits') to the MS 1-bit TGA format
//
// Preconditions:
//
// Parameters:	inSource,	contains the bits data to convert
//
//				outDestination,	where the TGA data will be written
//
//				bitsRowBytes,	number of bytes in one row of
//								bits data
//
//				msheader,	contains information about the TGA
//							dimensions and filesize
//
// Postconditions:
//
// Returns: B_ERROR,	if memory couldn't be allocated or another
//						error occured
//
// B_OK,	if no errors occurred
// ---------------------------------------------------------------
status_t
translate_from_bits1_to_tgabw(BPositionIO *inSource,
	BPositionIO *outDestination, int32 bitsRowBytes, TGAImageSpec &imagespec)
{
	int32 tgaRowBytes = imagespec.width;
	uint32 tgapixrow = 0;
	uint8 *tgaRowData = new uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		return B_ERROR;
	}
	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	while (rd == bitsRowBytes) {
		uint32 tgapixcol = 0;
		for (int32 i = 0; (tgapixcol < imagespec.width) && 
			(i < bitsRowBytes); i++) {
			// process each byte in the row
			uint8 pixels = bitsRowData[i];
			for (uint8 compbit = 128; (tgapixcol < imagespec.width) &&
				compbit; compbit >>= 1) {
				// for each bit in the current byte, convert to a TGA palette
				// index and store that in the tgaRowData
				if (pixels & compbit)
					// black
					tgaRowData[tgapixcol] = 0;
				else
					// white
					tgaRowData[tgapixcol] = 255;
				tgapixcol++;
			}
		}
				
		outDestination->Write(tgaRowData, tgaRowBytes);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (tgapixrow == imagespec.height)
			break;

		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)
	
	delete[] tgaRowData;
	delete[] bitsRowData;

	return B_OK;
}

// ---------------------------------------------------------------
// write_tga_headers
//
// Writes the MS TGA headers (fileHeader and msheader)
// to outDestination.
//
// Preconditions:
//
// Parameters:	outDestination,	where the headers are written to
//
//
// Postconditions:
//
// Returns: B_ERROR, if something went wrong
//
// B_OK, if there were no problems writing out the headers
// ---------------------------------------------------------------
status_t
write_tga_headers(BPositionIO *outDestination, TGAFileHeader &fileheader,
	TGAColorMapSpec &mapspec, TGAImageSpec &imagespec)
{
	uint8 tgaheaders[TGA_HEADERS_SIZE];
	
	// Convert host format headers to Little Endian (Intel) byte order
	TGAFileHeader outFileheader;
	outFileheader.idlength = fileheader.idlength;
	outFileheader.colormaptype = fileheader.colormaptype;
	outFileheader.imagetype = fileheader.imagetype;
	
	TGAColorMapSpec outMapspec;
	outMapspec.firstentry = B_HOST_TO_LENDIAN_INT16(mapspec.firstentry);
	outMapspec.length = B_HOST_TO_LENDIAN_INT16(mapspec.length);
	outMapspec.entrysize = mapspec.entrysize;
	
	TGAImageSpec outImagespec;
	outImagespec.xorigin = B_HOST_TO_LENDIAN_INT16(imagespec.xorigin);
	outImagespec.yorigin = B_HOST_TO_LENDIAN_INT16(imagespec.yorigin);
	outImagespec.width = B_HOST_TO_LENDIAN_INT16(imagespec.width);
	outImagespec.height = B_HOST_TO_LENDIAN_INT16(imagespec.height);
	outImagespec.depth = imagespec.depth;
	outImagespec.descriptor = imagespec.descriptor;
	
	// Copy TGA headers to buffer to be written out
	// all at once
	memcpy(tgaheaders, &outFileheader.idlength, 1);
	memcpy(tgaheaders + 1, &outFileheader.colormaptype, 1);
	memcpy(tgaheaders + 2, &outFileheader.imagetype, 1);
	
	memcpy(tgaheaders + 3, &outMapspec.firstentry, 2);
	memcpy(tgaheaders + 5, &outMapspec.length, 2);
	memcpy(tgaheaders + 7, &outMapspec.entrysize, 1);
	
	memcpy(tgaheaders + 8, &outImagespec.xorigin, 2);
	memcpy(tgaheaders + 10, &outImagespec.yorigin, 2);
	memcpy(tgaheaders + 12, &outImagespec.width, 2);
	memcpy(tgaheaders + 14, &outImagespec.height, 2);
	memcpy(tgaheaders + 16, &outImagespec.depth, 1);
	memcpy(tgaheaders + 17, &outImagespec.descriptor, 1);
	
	ssize_t written;
	written = outDestination->Write(tgaheaders, TGA_HEADERS_SIZE);
	
	if (written == TGA_HEADERS_SIZE)
		return B_OK;
	else
		return B_ERROR;
}

// ---------------------------------------------------------------
// write_tga_footer
//
// Writes the MS TGA headers (fileHeader and msheader)
// to outDestination.
//
// Preconditions:
//
// Parameters:	outDestination,	where the headers are written to
//
//
// Postconditions:
//
// Returns: B_ERROR, if something went wrong
//
// B_OK, if there were no problems writing out the headers
// ---------------------------------------------------------------
status_t
write_tga_footer(BPositionIO *outDestination)
{
	const int32 kfootersize = 26;
	uint8 footer[kfootersize];
	
	memset(footer, 0, 8);
	// copy the string including the '.' and the '\0'
	memcpy(footer + 8, "TRUEVISION-XFILE.", 18);
	
	ssize_t written;
	written = outDestination->Write(footer, kfootersize);
	if (written == kfootersize)
		return B_OK;
	else
		return B_ERROR;
}

// ---------------------------------------------------------------
// translate_from_bits
//
// Convert the data in inSource from the Be Bitmap format ('bits')
// to the format specified in outType (either bits or TGA).
//
// Preconditions:
//
// Parameters:	inSource,	the bits data to translate
//
// 				amtread,	the amount of data already read from
//							inSource
//
//				read,		pointer to the data already read from
//							inSource
//
//				bheaderonly,	true if only the header should be
//								written out
//
//				bdataonly,	true if only the data should be
//							written out
//
//				outType,	the type of data to convert to
//
//				outDestination,	where the output is written to
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data is not in a supported
//								format
//
// B_ERROR, if there was an error allocating memory or some other
//			error
//
// B_OK, if successfully translated the data from the bits format
// ---------------------------------------------------------------
status_t
translate_from_bits(BPositionIO *inSource, ssize_t amtread, uint8 *read,
	bool bheaderonly, bool bdataonly, uint32 outType,
	BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;
		
	status_t result;
	result = identify_bits_header(inSource, NULL, amtread, read, &bitsHeader);
	if (result != B_OK)
		return result;
	
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
			uint8 buf[1024];
			ssize_t rd = inSource->Read(buf, 1024);
			while (rd > 0) {
				outDestination->Write(buf, rd);
				rd = inSource->Read(buf, 1024);
			}
		
			if (rd == 0)
				return B_OK;
			else
				return B_ERROR;
		} else
			return B_OK;
		
	// Translate B_TRANSLATOR_BITMAP to B_TGA_FORMAT
	} else if (outType == B_TGA_FORMAT) {
		// Set up TGA header
		TGAFileHeader fileheader;
		fileheader.idlength = 0;
		fileheader.colormaptype = TGA_NO_COLORMAP;
		fileheader.imagetype = 0;
		
		TGAColorMapSpec mapspec;
		mapspec.firstentry = 0;
		mapspec.length = 0;
		mapspec.entrysize = 0;
		
		TGAImageSpec imagespec;
		imagespec.xorigin = 0;
		imagespec.yorigin = 0;
		imagespec.width = bitsHeader.bounds.Width() + 1;
		imagespec.height = bitsHeader.bounds.Height() + 1;
		imagespec.depth = 0;
		imagespec.descriptor = TGA_ORIGIN_VERT_BIT;
		
		// determine fileSize / imagesize
		switch (bitsHeader.colors) {
		
			// Output to 32-bit True Color TGA (8 bits alpha)
			case B_RGBA32:
			case B_RGBA32_BIG:
			case B_CMYA32:
				fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 32;
				imagespec.descriptor |= 8;
					// 8 bits of alpha
				break;
			
			// Output to 24-bit True Color TGA (no alpha)
			case B_RGB32:
			case B_RGB32_BIG:
			case B_RGB24:
			case B_RGB24_BIG:
			case B_CMYK32:
			case B_CMY32:
			case B_CMY24:
				fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 24;
				break;
		
			// Output to 16-bit True Color TGA (no alpha)
			// (TGA doesn't see 16 bit images as Be does
			// so converting 16 bit Be Image to 16-bit TGA
			// image would result in loss of quality)
			case B_RGB16:
			case B_RGB16_BIG:	
				fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 24;
				break;
			
			// Output to 15-bit True Color TGA (1 bit alpha)
			case B_RGB15:
			case B_RGB15_BIG:
				fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 16;
				imagespec.descriptor |= 1;
					// 1 bit of alpha (always opaque)
				break;
				
			// Output to 16-bit True Color TGA (1 bit alpha)
			case B_RGBA15:
			case B_RGBA15_BIG:
				fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 16;
				imagespec.descriptor |= 1;
					// 1 bit of alpha
				break;
				
			// Output to 8-bit Color Mapped TGA 32 bits per color map entry
			case B_CMAP8:
				fileheader.colormaptype = TGA_COLORMAP;
				fileheader.imagetype = TGA_NOCOMP_COLORMAP;
				mapspec.firstentry = 0;
				mapspec.length = 256;
				mapspec.entrysize = 32;
				imagespec.depth = 8;
				imagespec.descriptor |= 8;
					// the pixel values contain 8 bits of attribute data
				break;
				
			// Output to 8-bit Black and White TGA
			case B_GRAY8:
			case B_GRAY1:
				fileheader.imagetype = TGA_NOCOMP_BW;
				imagespec.depth = 8;
				break;
				
			default:
				return B_NO_TRANSLATOR;
		}
		
		// write out the TGA headers
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			result = write_tga_headers(outDestination, fileheader,
				mapspec, imagespec);
			if (result != B_OK)
				return result;
		}
		if (bheaderonly)
			// if user only wants the header, bail out
			// before the data is written
			return result;

		// write out the TGA pixel data
		switch (bitsHeader.colors) {
			case B_RGB32:
			case B_RGB32_BIG:
			case B_RGBA32:
			case B_RGBA32_BIG:
			case B_RGB24:
			case B_RGB24_BIG:
			case B_RGB16:
			case B_RGB16_BIG:
			case B_RGB15:
			case B_RGB15_BIG:
			case B_RGBA15:
			case B_RGBA15_BIG:
			case B_CMYK32:
			case B_CMY32:
			case B_CMYA32:
			case B_CMY24:
				result = translate_from_bits_to_tgatc(inSource, outDestination,
					bitsHeader.colors, imagespec);
				break;
					
			case B_CMAP8:
			{
				// write Be's system palette to the TGA file
				uint8 pal[1024];
				const color_map *pmap = system_colors();
				for (int32 i = 0; i < 256; i++) {
					uint8 *palent = pal + (i * 4);
					rgb_color c = pmap->color_list[i];
					palent[0] = c.blue;
					palent[1] = c.green;
					palent[2] = c.red;
					palent[3] = c.alpha;
				}
				if (outDestination->Write(pal, 1024) != 1024)
					return B_ERROR;

				result = translate_from_bits8_to_tga8(inSource, outDestination,
					bitsHeader.rowBytes, imagespec.height);
				break;
			}
			
			case B_GRAY8:
				result = translate_from_bits8_to_tga8(inSource, outDestination,
					bitsHeader.rowBytes, imagespec.height);
				break;
				
			case B_GRAY1:
				result = translate_from_bits1_to_tgabw(inSource, outDestination,
					bitsHeader.rowBytes, imagespec);
				break;
				
			default:
				result = B_NO_TRANSLATOR;
				break;
		}
		
		if (result == B_OK)
			result = write_tga_footer(outDestination);
			
		return result;

	} else
		return B_NO_TRANSLATOR;
}

// ---------------------------------------------------------------
// translate_from_tgatc_to_bits
//
// Translates a non-palette TGA from inSource to the B_RGB32
// bits format.
//
// Preconditions:
//
// Parameters: inSource,	the TGA data to be translated
//
// outDestination,	where the bits data will be written to
//
//
// Postconditions:
//
// Returns: B_ERROR, if there is an error allocating memory
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
translate_from_tgatc_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination, TGAImageSpec &imagespec)
{
	bool bvflip;
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		bvflip = false;
	else
		bvflip = true;
	uint8 nalpha = imagespec.descriptor & TGA_DESC_ALPHABITS;

	int32 bitsRowBytes = imagespec.width * 4;
	int32 tgaBytesPerPixel = imagespec.depth / 8;
	if (imagespec.depth % 8)
		tgaBytesPerPixel++;
	int32 tgaRowBytes = (imagespec.width * tgaBytesPerPixel);
	uint32 tgapixrow = 0;
	
	// Setup outDestination so that it can be written to
	// from the end of the file to the beginning instead of
	// the other way around
	off_t bitsFileSize = (bitsRowBytes * imagespec.height) + 
		sizeof(TranslatorBitmap);
	if (outDestination->SetSize(bitsFileSize) != B_OK)
		// This call should work for BFile and BMallocIO objects,
		// but may not work for other BPositionIO based types
		return B_ERROR;
	off_t bitsoffset = (imagespec.height - 1) * bitsRowBytes;
	if (bvflip)
		outDestination->Seek(bitsoffset, SEEK_CUR);
	
	// allocate row buffers
	uint8 *tgaRowData = new uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		return B_ERROR;
	}

	// perform the actual translation
	memset(bitsRowData, 0xff, bitsRowBytes);
	ssize_t rd = inSource->Read(tgaRowData, tgaRowBytes);
	while (rd == tgaRowBytes) {
		uint8 *pbitspixel = bitsRowData;
		uint8 *ptgapixel = tgaRowData;
		uint16 val;
		for (uint32 i = 0; i < imagespec.width; i++) {
			switch (imagespec.depth) {
				case 32:
					if (nalpha == 8)
						memcpy(pbitspixel, ptgapixel, 4);
					else {
						memcpy(pbitspixel, ptgapixel, 3);
						pbitspixel[3] = 255;
					}
					break;
				case 24:
					memcpy(pbitspixel, ptgapixel, 3);
					pbitspixel[3] = 255;
					break;
				case 16:
					val = ptgapixel[0] + (ptgapixel[1] << 8);
					pbitspixel[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					pbitspixel[1] =
						((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					pbitspixel[2] =
						((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
					if (nalpha == 1)
						pbitspixel[3] = (val & 0x8000) ? 255 : 0;
					else
						pbitspixel[3] = 255;
					break;
				case 15:
					val = ptgapixel[0] + (ptgapixel[1] << 8);
					pbitspixel[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					pbitspixel[1] =
						((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					pbitspixel[2] =
						((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
					pbitspixel[3] = 255;
					break;
				default:
					break;
			}
			pbitspixel += 4;
			ptgapixel += tgaBytesPerPixel;
		}
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (tgapixrow == imagespec.height)
			break;

		if (bvflip)
			outDestination->Seek(-(bitsRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(tgaRowData, tgaRowBytes);
	}
	
	delete[] tgaRowData;
	delete[] bitsRowData;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_tgacm_to_bits
//
// Translates a non-palette TGA from inSource to the B_RGB32
// bits format.
//
// Preconditions:
//
// Parameters: inSource,	the TGA data to be translated
//
// outDestination,	where the bits data will be written to
//
//
// Postconditions:
//
// Returns: B_ERROR, if there is an error allocating memory
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
translate_from_tgacm_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination, TGAColorMapSpec &mapspec,
	TGAImageSpec &imagespec, uint8 *pmap)
{
	bool bvflip;
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		bvflip = false;
	else
		bvflip = true;
	uint8 nalpha = imagespec.descriptor & TGA_DESC_ALPHABITS;

	int32 bitsRowBytes = imagespec.width * 4;
	int32 tgaPalBytesPerPixel = mapspec.entrysize / 8;
	if (mapspec.entrysize % 8)
		tgaPalBytesPerPixel++;
	int32 tgaBytesPerPixel = imagespec.depth / 8;
	if (imagespec.depth % 8)
		tgaBytesPerPixel++;
	int32 tgaRowBytes = (imagespec.width * tgaBytesPerPixel);
	uint32 tgapixrow = 0;
	
	// Setup outDestination so that it can be written to
	// from the end of the file to the beginning instead of
	// the other way around
	off_t bitsFileSize = (bitsRowBytes * imagespec.height) + 
		sizeof(TranslatorBitmap);
	if (outDestination->SetSize(bitsFileSize) != B_OK)
		// This call should work for BFile and BMallocIO objects,
		// but may not work for other BPositionIO based types
		return B_ERROR;
	off_t bitsoffset = (imagespec.height - 1) * bitsRowBytes;
	if (bvflip)
		outDestination->Seek(bitsoffset, SEEK_CUR);
	
	// allocate row buffers
	uint8 *tgaRowData = new uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		return B_ERROR;
	}

	// perform the actual translation
	memset(bitsRowData, 0xff, bitsRowBytes);
	ssize_t rd = inSource->Read(tgaRowData, tgaRowBytes);
	while (rd == tgaRowBytes) {
		uint8 *pbitspixel = bitsRowData;
		uint8 *ptgapixel = NULL;
		uint16 val;
		for (uint32 i = 0; i < imagespec.width; i++) {
			ptgapixel = pmap +
					(tgaRowData[i] * tgaPalBytesPerPixel);
			switch (mapspec.entrysize) {
				case 32:
					memcpy(pbitspixel, ptgapixel, 4);
					break;
					
				case 24:
					memcpy(pbitspixel, ptgapixel, 3);
					pbitspixel[3] = 255;
					break;
					
				case 16:
					val = ptgapixel[0] + (ptgapixel[1] << 8);
					pbitspixel[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					pbitspixel[1] =
						((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					pbitspixel[2] =
						((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
					if (nalpha == 1)
						pbitspixel[3] = (val & 0x8000) ? 255 : 0;
					else
						pbitspixel[3] = 255;
					break;
					
				case 15:
					val = ptgapixel[0] + (ptgapixel[1] << 8);
					pbitspixel[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					pbitspixel[1] =
						((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					pbitspixel[2] =
						((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
					pbitspixel[3] = 255;
					break;
					
				default:
					break;
			}
			pbitspixel += 4;
		}
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (tgapixrow == imagespec.height)
			break;

		if (bvflip)
			outDestination->Seek(-(bitsRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(tgaRowData, tgaRowBytes);
	}
	
	delete[] tgaRowData;
	delete[] bitsRowData;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_tga
//
// Convert the data in inSource from the TGA format
// to the format specified in outType (either bits or TGA).
//
// Preconditions:
//
// Parameters:	inSource,	the bits data to translate
//
// 				amtread,	the amount of data already read from
//							inSource
//
//				read,		pointer to the data already read from
//							inSource
//
//				bheaderonly,	true if only the header should be
//								written out
//
//				bdataonly,	true if only the data should be
//							written out
//
//				outType,	the type of data to convert to
//
//				outDestination,	where the output is written to
//
// Postconditions:
//
// Returns: B_NO_TRANSLATOR,	if the data is not in a supported
//								format
//
// B_ERROR, if there was an error allocating memory or some other
//			error
//
// B_OK, if successfully translated the data from the bits format
// ---------------------------------------------------------------
status_t
translate_from_tga(BPositionIO *inSource, ssize_t amtread, uint8 *read,
	bool bheaderonly, bool bdataonly, uint32 outType,
	BPositionIO *outDestination)
{
	TGAFileHeader fileheader;
	TGAColorMapSpec mapspec;
	TGAImageSpec imagespec;

	status_t result;
	result = identify_tga_header(inSource, NULL, amtread, read,
		&fileheader, &mapspec, &imagespec);
	if (result != B_OK)
		return result;
	
	// if the user wants to translate a TGA to a TGA, easy enough :)	
	if (outType == B_TGA_FORMAT) {
		// write out the TGA headers
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			result = write_tga_headers(outDestination, fileheader,
				mapspec, imagespec);
			if (result != B_OK)
				return result;
		}
		if (bheaderonly)
			// if the user only wants the header, 
			// bail before it is written
			return result;
			
		uint8 buf[1024];
		ssize_t rd = inSource->Read(buf, rd);
		while (rd > 0) {
			outDestination->Write(buf, rd);
			rd = inSource->Read(buf, rd);
		}
		if (rd == 0)
			return B_OK;
		else
			return B_ERROR;
	
	// if translating a TGA to a Be Bitmap
	} else if (outType == B_TRANSLATOR_BITMAP) {
		TranslatorBitmap bitsHeader;
		bitsHeader.magic = B_TRANSLATOR_BITMAP;
		bitsHeader.bounds.left = 0;
		bitsHeader.bounds.top = 0;
		bitsHeader.bounds.right = imagespec.width - 1;
		bitsHeader.bounds.bottom = imagespec.height - 1;
		
		// skip over Image ID data (if present)
		if (fileheader.idlength > 0)
			inSource->Seek(fileheader.idlength, SEEK_CUR);
		
		// read in palette and/or skip non-TGA data					
		uint8 *ptgapalette = NULL;
		if (fileheader.colormaptype == TGA_COLORMAP) {
			uint32 nentrybytes;
			nentrybytes = mapspec.entrysize / 8;
			if (mapspec.entrysize % 8)
				nentrybytes++;
			ptgapalette = new uint8[nentrybytes * mapspec.length];
			inSource->Read(ptgapalette, nentrybytes * mapspec.length);
		}

		bitsHeader.rowBytes = imagespec.width * 4;
		if (imagespec.descriptor & TGA_DESC_ALPHABITS)
			bitsHeader.colors = B_RGBA32;
		else
			bitsHeader.colors = B_RGB32;
		int32 datasize = bitsHeader.rowBytes * imagespec.height;
		bitsHeader.dataSize = datasize;
				
		// write out Be's Bitmap header
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			if (swap_data(B_UINT32_TYPE, &bitsHeader,
				sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK)
				return B_ERROR;
			outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
		}
		if (bheaderonly)
			// if the user only wants the header, 
			// bail before the data is written
			return B_OK;
		
		// write out the actual image data
		switch (fileheader.imagetype) {
			case TGA_NOCOMP_TRUECOLOR:
				result = translate_from_tgatc_to_bits(inSource,
					outDestination, imagespec);
				break;
				
			case TGA_NOCOMP_COLORMAP:
				result = translate_from_tgacm_to_bits(inSource,
					outDestination, mapspec, imagespec, ptgapalette);
				break;
				
			default:
				result = B_NO_TRANSLATOR;
				break;
		}
		
		if (ptgapalette) {
			delete[] ptgapalette;
			ptgapalette = NULL;
		}
		
		return result;
		
	} else
		return B_NO_TRANSLATOR;
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
TGATranslator::Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_TGA_FORMAT)
		return B_NO_TRANSLATOR;
		
	inSource->Seek(0, SEEK_SET);
	
	uint8 ch[4];
	uint32 nbits = B_TRANSLATOR_BITMAP;
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	if (swap_data(B_UINT32_TYPE, &nbits, sizeof(uint32),
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
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
		return translate_from_bits(inSource, 4, ch, bheaderonly, bdataonly,
			outType, outDestination);
	// If NOT B_TRANSLATOR_BITMAP type, 
	// it could be the TGA format
	else
		return translate_from_tga(inSource, 4, ch, bheaderonly, bdataonly,
			outType, outDestination);
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
// Returns:
// ---------------------------------------------------------------
status_t
TGATranslator::MakeConfigurationView(BMessage *ioExtension, BView **outView,
	BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	TGAView *view = new TGAView(BRect(0, 0, 225, 175),
		"TGATranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	*outView = view;
	*outExtent = view->Bounds();

	return B_OK;
}
