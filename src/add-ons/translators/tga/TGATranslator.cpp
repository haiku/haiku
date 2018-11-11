/*****************************************************************************/
// TGATranslator
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TGATranslator.cpp
//
// This BTranslator based object is for opening and writing TGA files.
//
//
// Copyright (c) 2002-2009, Haiku, Inc. All rights reserved.
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

#include <Catalog.h>

#include "TGATranslator.h"
#include "TGAView.h"
#include "StreamBuffer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TGATranslator"

// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
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
static const translation_format sOutputFormats[] = {
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

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false},
	{TGA_SETTING_RLE, TRAN_SETTING_BOOL, false},
		// RLE compression is off by default
	{TGA_SETTING_IGNORE_ALPHA, TRAN_SETTING_BOOL, false}
		// Don't ignore the alpha channel by default
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


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
	BTranslator *ptranslator = NULL;
	if (!n)
		ptranslator = new(std::nothrow) TGATranslator();

	return ptranslator;
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
	: BaseTranslator(B_TRANSLATE("TGA images"),
		B_TRANSLATE("TGA image translator"),
		TGA_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"TGATranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, B_TGA_FORMAT)
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
//
// NOTE: It may be the case, that under Be's libtranslation.so,
// that this destructor will never be called
TGATranslator::~TGATranslator()
{
}

uint8
TGATranslator::tga_alphabits(TGAFileHeader &filehead, TGAColorMapSpec &mapspec,
	TGAImageSpec &imagespec)
{
	if (fSettings->SetGetBool(TGA_SETTING_IGNORE_ALPHA))
		return 0;
	else {
		uint8 nalpha;
		if (filehead.imagetype == TGA_NOCOMP_COLORMAP ||
			filehead.imagetype == TGA_RLE_COLORMAP) {
			// color mapped images

			if (mapspec.entrysize == 32)
				nalpha = 8;
			else if (mapspec.entrysize == 16)
				nalpha = 1;
			else
				nalpha = 0;

		} else {
			// non-color mapped images

			if (imagespec.depth == 32)
				// Some programs that generate 32-bit TGA files
				// have an alpha channel, but have an incorrect
				// descriptor which says there are no alpha bits.
				// This logic is so that the alpha data can be
				// obtained from TGA files that lie.
				nalpha = 8;
			else
				nalpha = imagespec.descriptor & TGA_DESC_ALPHABITS;
		}

		return nalpha;
	}
}

// ---------------------------------------------------------------
// identify_tga_header
//
// Determines if the data in inSource is in the TGA format.
// If it is, it returns info about the data in inSource
// to outInfo, pfileheader, pmapspec and pimagespec.
//
// Preconditions:
//
// Parameters:	inSource,	The source of the image data
//
//				outInfo,	Information about the translator
//							is copied here
//
//				pfileheader,	File header info for the TGA is
//								copied here after it is read from
//								the file.
//
//				pmapspec,	color map info for the TGA is copied
//							here after it is read from the file
//
//				pimagespec,	Info about the image width/height etc.
//							is copied here after it is read from
//							the file
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
	TGAFileHeader *pfileheader = NULL, TGAColorMapSpec *pmapspec = NULL,
	TGAImageSpec *pimagespec = NULL)
{
	uint8 buf[TGA_HEADERS_SIZE];

	// read in the rest of the TGA headers
	ssize_t size = TGA_HEADERS_SIZE;
	if (size > 0 && inSource->Read(buf, size) != size)
		return B_NO_TRANSLATOR;

	// Read in TGA file header
	TGAFileHeader fileheader;
	fileheader.idlength = buf[0];

	fileheader.colormaptype = buf[1];
	if (fileheader.colormaptype > 1)
		return B_NO_TRANSLATOR;

	fileheader.imagetype = buf[2];
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

	mapspec.entrysize = buf[7];
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

	imagespec.depth = buf[16];
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

	imagespec.descriptor = buf[17];
	// images ordered from Right to Left (rather than Left to Right)
	// are not supported
	if ((imagespec.descriptor & TGA_ORIGIN_HORZ_BIT) != TGA_ORIGIN_LEFT)
		return B_NO_TRANSLATOR;
	// unused descriptor bits, these bits must be zero
	if (imagespec.descriptor & TGA_DESC_BITS76)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 32 &&
		(imagespec.descriptor & TGA_DESC_ALPHABITS) != 8 &&
		(imagespec.descriptor & TGA_DESC_ALPHABITS) != 0)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 24 &&
		(imagespec.descriptor & TGA_DESC_ALPHABITS) != 0)
		return B_NO_TRANSLATOR;
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 16 &&
		(imagespec.descriptor & TGA_DESC_ALPHABITS) != 1 &&
		(imagespec.descriptor & TGA_DESC_ALPHABITS) != 0)
	if ((fileheader.imagetype == TGA_NOCOMP_TRUECOLOR ||
		fileheader.imagetype == TGA_RLE_TRUECOLOR) &&
		imagespec.depth == 15 &&
		(imagespec.descriptor & TGA_DESC_ALPHABITS) != 0)
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
		switch (fileheader.imagetype) {
			case TGA_NOCOMP_COLORMAP:
				snprintf(outInfo->name, sizeof(outInfo->name),
					B_TRANSLATE("Targa image (%d bits colormap)"),
					imagespec.depth);
				break;
			case TGA_NOCOMP_TRUECOLOR:
				snprintf(outInfo->name, sizeof(outInfo->name),
					B_TRANSLATE("Targa image (%d bits truecolor)"),
					imagespec.depth);
				break;
			case TGA_RLE_COLORMAP:
				snprintf(outInfo->name, sizeof(outInfo->name),
					B_TRANSLATE("Targa image (%d bits RLE colormap)"),
					imagespec.depth);
				break;
			case TGA_RLE_TRUECOLOR:
				snprintf(outInfo->name, sizeof(outInfo->name),
					B_TRANSLATE("Targa image (%d bits RLE truecolor)"),
					imagespec.depth);
				break;
			case TGA_RLE_BW:
				snprintf(outInfo->name, sizeof(outInfo->name),
					B_TRANSLATE("Targa image (%d bits RLE gray)"),
					imagespec.depth);
				break;
			case TGA_NOCOMP_BW:
			default:
				snprintf(outInfo->name, sizeof(outInfo->name),
					B_TRANSLATE("Targa image (%d bits gray)"),
					imagespec.depth);
				break;

		}
		strcpy(outInfo->MIME, "image/x-targa");
	}

	return B_OK;
}

status_t
TGATranslator::DerivedIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	return identify_tga_header(inSource, outInfo);
}

// Convert width pixels from pbits to TGA format, storing the
// result in ptga
status_t
pix_bits_to_tga(uint8 *pbits, uint8 *ptga, color_space fromspace,
	uint16 width, const color_map *pmap, int32 bitsBytesPerPixel)
{
	status_t bytescopied = 0;

	switch (fromspace) {
		case B_RGBA32:
			bytescopied = width * 4;
			memcpy(ptga, pbits, bytescopied);
			break;

		case B_RGBA32_BIG:
			bytescopied = width * 4;
			while (width--) {
				ptga[0] = pbits[3];
				ptga[1] = pbits[2];
				ptga[2] = pbits[1];
				ptga[3] = pbits[0];

				ptga += 4;
				pbits += 4;
			}
			break;

		case B_CMYA32:
			bytescopied = width * 4;
			while (width--) {
				ptga[0] = 255 - pbits[2];
				ptga[1] = 255 - pbits[1];
				ptga[2] = 255 - pbits[0];
				ptga[3] = pbits[3];

				ptga += 4;
				pbits += 4;
			}
			break;

		case B_RGB32:
		case B_RGB24:
			bytescopied = width * 3;
			while (width--) {
				memcpy(ptga, pbits, 3);

				ptga += 3;
				pbits += bitsBytesPerPixel;
			}
			break;

		case B_CMYK32:
		{
			int32 comp;
			bytescopied = width * 3;
			while (width--) {
				comp = 255 - pbits[2] - pbits[3];
				ptga[0] = (comp < 0) ? 0 : comp;

				comp = 255 - pbits[1] - pbits[3];
				ptga[1] = (comp < 0) ? 0 : comp;

				comp = 255 - pbits[0] - pbits[3];
				ptga[2] = (comp < 0) ? 0 : comp;

				ptga += 3;
				pbits += 4;
			}
			break;
		}

		case B_CMY32:
		case B_CMY24:
			bytescopied = width * 3;
			while (width--) {
				ptga[0] = 255 - pbits[2];
				ptga[1] = 255 - pbits[1];
				ptga[2] = 255 - pbits[0];

				ptga += 3;
				pbits += bitsBytesPerPixel;
			}
			break;

		case B_RGB16:
		case B_RGB16_BIG:
		{
			// Expand to 24 bit because the TGA format handles
			// 16 bit images differently than the Be Image Format
			// which would cause a loss in quality
			uint16 val;
			bytescopied = width * 3;
			while (width--) {
				if (fromspace == B_RGB16)
					val = pbits[0] + (pbits[1] << 8);
				else
					val = pbits[1] + (pbits[0] << 8);

				ptga[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				ptga[1] =
					((val & 0x7e0) >> 3) | ((val & 0x7e0) >> 9);
				ptga[2] =
					((val & 0xf800) >> 8) | ((val & 0xf800) >> 13);

				ptga += 3;
				pbits += 2;
			}
			break;
		}

		case B_RGBA15:
			bytescopied = width * 2;
			memcpy(ptga, pbits, bytescopied);
			break;

		case B_RGBA15_BIG:
			bytescopied = width * 2;
			while (width--) {
				ptga[0] = pbits[1];
				ptga[1] = pbits[0];

				ptga += 2;
				pbits += 2;
			}
			break;

		case B_RGB15:
			bytescopied = width * 2;
			while (width--) {
				ptga[0] = pbits[0];
				ptga[1] = pbits[1] | 0x80;
					// alpha bit is always 1

				ptga += 2;
				pbits += 2;
			}
			break;

		case B_RGB15_BIG:
			bytescopied = width * 2;
			while (width--) {
				ptga[0] = pbits[1];
				ptga[1] = pbits[0] | 0x80;
					// alpha bit is always 1

				ptga += 2;
				pbits += 2;
			}
			break;

		case B_RGB32_BIG:
			bytescopied = width * 3;
			while (width--) {
				ptga[0] = pbits[3];
				ptga[1] = pbits[2];
				ptga[2] = pbits[1];

				ptga += 3;
				pbits += 4;
			}
			break;

		case B_RGB24_BIG:
			bytescopied = width * 3;
			while (width--) {
				ptga[0] = pbits[2];
				ptga[1] = pbits[1];
				ptga[2] = pbits[0];

				ptga += 3;
				pbits += 3;
			}
			break;

		case B_CMAP8:
		{
			rgb_color c;
			bytescopied = width * 3;
			while (width--) {
				c = pmap->color_list[pbits[0]];
				ptga[0] = c.blue;
				ptga[1] = c.green;
				ptga[2] = c.red;

				ptga += 3;
				pbits++;
			}
			break;
		}

		case B_GRAY8:
			// NOTE: this code assumes that the
			// destination TGA color space is either
			// 8 bit indexed color or 8 bit grayscale
			bytescopied = width;
			memcpy(ptga, pbits, bytescopied);
			break;

		default:
			bytescopied = B_ERROR;
			break;
	} // switch (fromspace)

	return bytescopied;
}

// create a TGA RLE packet for pixel and copy the
// packet header and pixel data to ptga
status_t
copy_rle_packet(uint8 *ptga, uint32 pixel, uint8 count,
	color_space fromspace, const color_map *pmap,
	int32 bitsBytesPerPixel)
{
	// copy packet header
	// (made of type and count)
	uint8 packethead = (count - 1) | 0x80;
	ptga[0] = packethead;
	ptga++;

	return pix_bits_to_tga(reinterpret_cast<uint8 *> (&pixel),
		ptga, fromspace, 1, pmap, bitsBytesPerPixel) + 1;
}

// create a TGA raw packet for pixel and copy the
// packet header and pixel data to ptga
status_t
copy_raw_packet(uint8 *ptga, uint8 *praw, uint8 count,
	color_space fromspace, const color_map *pmap,
	int32 bitsBytesPerPixel)
{
	// copy packet header
	// (made of type and count)
	uint8 packethead = count - 1;
	ptga[0] = packethead;
	ptga++;

	return pix_bits_to_tga(praw, ptga, fromspace,
		count, pmap, bitsBytesPerPixel) + 1;
}

// convert a row of pixel data from pbits to a
// row of pixel data in the TGA format using
// Run Length Encoding
status_t
pix_bits_to_tgarle(uint8 *pbits, uint8 *ptga, color_space fromspace,
	uint16 width, const color_map *pmap, int32 bitsBytesPerPixel)
{
	if (width == 0)
		return B_ERROR;

	uint32 current = 0, next = 0, aftnext = 0;
	uint16 nread = 0;
	status_t result, bytescopied = 0;
	uint8 *prawbuf, *praw;
	prawbuf = new(std::nothrow) uint8[bitsBytesPerPixel * 128];
	praw = prawbuf;
	if (!prawbuf)
		return B_ERROR;

	uint8 rlecount = 1, rawcount = 0;
	bool bJustWroteRLE = false;

	memcpy(&current, pbits, bitsBytesPerPixel);
	pbits += bitsBytesPerPixel;
	if (width == 1) {
		result = copy_raw_packet(ptga,
			reinterpret_cast<uint8 *> (&current), 1,
			fromspace, pmap, bitsBytesPerPixel);

		ptga += result;
		bytescopied += result;
		nread++;
			// don't enter the while loop

	} else {
		memcpy(&next, pbits, bitsBytesPerPixel);
		pbits += bitsBytesPerPixel;
		nread++;
	}

	while (nread < width) {

		if (nread < width - 1) {
			memcpy(&aftnext, pbits, bitsBytesPerPixel);
			pbits += bitsBytesPerPixel;
		}
		nread++;

		// RLE Packet Creation
		if (current == next && !bJustWroteRLE) {
			rlecount++;

			if (next != aftnext || nread == width || rlecount == 128) {
				result = copy_rle_packet(ptga, current, rlecount,
					fromspace, pmap, bitsBytesPerPixel);

				ptga += result;
				bytescopied += result;
				rlecount = 1;
				bJustWroteRLE = true;
			}

		// RAW Packet Creation
		} else {

			if (!bJustWroteRLE) {
				// output the current pixel only if
				// it was not just written out in an RLE packet
				rawcount++;
				memcpy(praw, &current, bitsBytesPerPixel);
				praw += bitsBytesPerPixel;
			}

			if (nread == width) {
				// if in the last iteration of the loop,
				// "next" will be the last pixel in the row,
				// and will need to be written out for this
				// special case

				if (rawcount == 128) {
					result = copy_raw_packet(ptga, prawbuf, rawcount,
						fromspace, pmap, bitsBytesPerPixel);

					ptga += result;
					bytescopied += result;
					praw = prawbuf;
					rawcount = 0;
				}

				rawcount++;
				memcpy(praw, &next, bitsBytesPerPixel);
				praw += bitsBytesPerPixel;
			}

			if ((!bJustWroteRLE && next == aftnext) ||
				nread == width || rawcount == 128) {
				result = copy_raw_packet(ptga, prawbuf, rawcount,
					fromspace, pmap, bitsBytesPerPixel);

				ptga += result;
				bytescopied += result;
				praw = prawbuf;
				rawcount = 0;
			}

			bJustWroteRLE = false;
		}

		current = next;
		next = aftnext;
	}

	delete[] prawbuf;
	prawbuf = NULL;

	return bytescopied;
}

// ---------------------------------------------------------------
// translate_from_bits_to_tgatc
//
// Converts various varieties of the Be Bitmap format ('bits') to
// the TGA True Color format (RLE or uncompressed)
//
// Preconditions:
//
// Parameters:	inSource,	contains the bits data to convert
//
//				outDestination,	where the TGA data will be written
//
//				fromspace,	the format of the data in inSource
//
//				imagespec,	info about width / height / etc. of
//							the image
//
//				brle,	output using RLE if true, uncompressed
//						if false
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
	BPositionIO *outDestination, color_space fromspace,
	TGAImageSpec &imagespec, bool brle)
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
	uint8 tgaBytesPerPixel = (imagespec.depth / 8) +
		((imagespec.depth % 8) ? 1 : 0);
	int32 tgaRowBytes = (imagespec.width * tgaBytesPerPixel) +
		(imagespec.width / 2);
	uint32 tgapixrow = 0;
	uint8 *tgaRowData = new(std::nothrow) uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new(std::nothrow) uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		tgaRowData = NULL;
		return B_ERROR;
	}

	// conversion function pointer, points to either
	// RLE or normal TGA conversion function
	status_t (*convert_to_tga)(uint8 *pbits, uint8 *ptga,
		color_space fromspace, uint16 width, const color_map *pmap,
		int32 bitsBytesPerPixel);

	if (brle)
		convert_to_tga = pix_bits_to_tgarle;
	else
		convert_to_tga = pix_bits_to_tga;

	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	const color_map *pmap = NULL;
	if (fromspace == B_CMAP8) {
		pmap = system_colors();
		if (!pmap) {
			delete[] tgaRowData;
			delete[] bitsRowData;
			return B_ERROR;
		}
	}
	while (rd == bitsRowBytes) {
		status_t bytescopied;
		bytescopied = convert_to_tga(bitsRowData, tgaRowData, fromspace,
			imagespec.width, pmap, bitsBytesPerPixel);

		outDestination->Write(tgaRowData, bytescopied);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read
		// non-pixel data
		if (tgapixrow == imagespec.height)
			break;

		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)

	delete[] bitsRowData;
	bitsRowData = NULL;
	delete[] tgaRowData;
	tgaRowData = NULL;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_bits1_to_tgabw
//
// Converts 1-bit Be Bitmaps ('bits') to the
// black and white (8-bit grayscale) TGA format
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
//				imagespec,	info about width / height / etc. of
//							the image
//
//				brle,	output using RLE if true, uncompressed
//						if false
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
translate_from_bits1_to_tgabw(BPositionIO *inSource,
	BPositionIO *outDestination, int32 bitsRowBytes,
	TGAImageSpec &imagespec, bool brle)
{
	uint8 tgaBytesPerPixel = 1;
	int32 tgaRowBytes = (imagespec.width * tgaBytesPerPixel) +
		(imagespec.width / 2);
	uint32 tgapixrow = 0;
	uint8 *tgaRowData = new(std::nothrow) uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;

	uint8 *medRowData = new(std::nothrow) uint8[imagespec.width];
	if (!medRowData) {
		delete[] tgaRowData;
		tgaRowData = NULL;
		return B_ERROR;
	}
	uint8 *bitsRowData = new(std::nothrow) uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] medRowData;
		medRowData = NULL;
		delete[] tgaRowData;
		tgaRowData = NULL;
		return B_ERROR;
	}

	// conversion function pointer, points to either
	// RLE or normal TGA conversion function
	status_t (*convert_to_tga)(uint8 *pbits, uint8 *ptga,
		color_space fromspace, uint16 width, const color_map *pmap,
		int32 bitsBytesPerPixel);

	if (brle)
		convert_to_tga = pix_bits_to_tgarle;
	else
		convert_to_tga = pix_bits_to_tga;

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
					medRowData[tgapixcol] = 0;
				else
					// white
					medRowData[tgapixcol] = 255;
				tgapixcol++;
			}
		}

		status_t bytescopied;
		bytescopied = convert_to_tga(medRowData, tgaRowData, B_GRAY8,
			imagespec.width, NULL, 1);

		outDestination->Write(tgaRowData, bytescopied);
		tgapixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read
		// non-pixel data
		if (tgapixrow == imagespec.height)
			break;

		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)

	delete[] bitsRowData;
	bitsRowData = NULL;
	delete[] medRowData;
	medRowData = NULL;
	delete[] tgaRowData;
	tgaRowData = NULL;

	return B_OK;
}

// ---------------------------------------------------------------
// write_tga_headers
//
// Writes the TGA headers to outDestination.
//
// Preconditions:
//
// Parameters:	outDestination,	where the headers are written to
//
//				fileheader, TGA file header
//
//				mapspec,	color map information
//
//				imagespec,	width / height / etc. info
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
	tgaheaders[0] = outFileheader.idlength;
	tgaheaders[1] = outFileheader.colormaptype;
	tgaheaders[2] = outFileheader.imagetype;

	memcpy(tgaheaders + 3, &outMapspec.firstentry, 2);
	memcpy(tgaheaders + 5, &outMapspec.length, 2);
	tgaheaders[7] = outMapspec.entrysize;

	memcpy(tgaheaders + 8, &outImagespec.xorigin, 2);
	memcpy(tgaheaders + 10, &outImagespec.yorigin, 2);
	memcpy(tgaheaders + 12, &outImagespec.width, 2);
	memcpy(tgaheaders + 14, &outImagespec.height, 2);
	tgaheaders[16] = outImagespec.depth;
	tgaheaders[17] = outImagespec.descriptor;

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
// Writes the TGA footer.  This information is contant in this
// code because this translator does not output the developer
// information section of the TGA format.
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
		// set the Extension Area Offset and Developer
		// Area Offset to zero (as they are not present)

	memcpy(footer + 8, "TRUEVISION-XFILE.", 18);
		// copy the string including the '.' and the '\0'

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
TGATranslator::translate_from_bits(BPositionIO *inSource, uint32 outType,
	BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;
	bool bheaderonly = false, bdataonly = false, brle;
	brle = fSettings->SetGetBool(TGA_SETTING_RLE);

	status_t result;
	result = identify_bits_header(inSource, NULL, &bitsHeader);
	if (result != B_OK)
		return result;

	// Translate B_TRANSLATOR_BITMAP to B_TGA_FORMAT
	if (outType == B_TGA_FORMAT) {
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
		imagespec.width = static_cast<uint16> (bitsHeader.bounds.Width() + 1);
		imagespec.height = static_cast<uint16> (bitsHeader.bounds.Height() + 1);
		imagespec.depth = 0;
		imagespec.descriptor = TGA_ORIGIN_VERT_BIT;

		// determine fileSize / imagesize
		switch (bitsHeader.colors) {

			// Output to 32-bit True Color TGA (8 bits alpha)
			case B_RGBA32:
			case B_RGBA32_BIG:
			case B_CMYA32:
				if (brle)
					fileheader.imagetype = TGA_RLE_TRUECOLOR;
				else
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
				if (brle)
					fileheader.imagetype = TGA_RLE_TRUECOLOR;
				else
					fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 24;
				break;

			// Output to 16-bit True Color TGA (no alpha)
			// (TGA doesn't see 16 bit images as Be does
			// so converting 16 bit Be Image to 16-bit TGA
			// image would result in loss of quality)
			case B_RGB16:
			case B_RGB16_BIG:
				if (brle)
					fileheader.imagetype = TGA_RLE_TRUECOLOR;
				else
					fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 24;
				break;

			// Output to 15-bit True Color TGA (1 bit alpha)
			case B_RGB15:
			case B_RGB15_BIG:
				if (brle)
					fileheader.imagetype = TGA_RLE_TRUECOLOR;
				else
					fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 16;
				imagespec.descriptor |= 1;
					// 1 bit of alpha (always opaque)
				break;

			// Output to 16-bit True Color TGA (1 bit alpha)
			case B_RGBA15:
			case B_RGBA15_BIG:
				if (brle)
					fileheader.imagetype = TGA_RLE_TRUECOLOR;
				else
					fileheader.imagetype = TGA_NOCOMP_TRUECOLOR;
				imagespec.depth = 16;
				imagespec.descriptor |= 1;
					// 1 bit of alpha
				break;

			// Output to 8-bit Color Mapped TGA 32 bits per color map entry
			case B_CMAP8:
				fileheader.colormaptype = TGA_COLORMAP;
				if (brle)
					fileheader.imagetype = TGA_RLE_COLORMAP;
				else
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
				if (brle)
					fileheader.imagetype = TGA_RLE_BW;
				else
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
					bitsHeader.colors, imagespec, brle);
				break;

			case B_CMAP8:
			{
				// write Be's system palette to the TGA file
				uint8 pal[1024];
				const color_map *pmap = system_colors();
				if (!pmap)
					return B_ERROR;
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

				result = translate_from_bits_to_tgatc(inSource, outDestination,
					B_GRAY8, imagespec, brle);
				break;
			}

			case B_GRAY8:
				result = translate_from_bits_to_tgatc(inSource, outDestination,
					B_GRAY8, imagespec, brle);
				break;

			case B_GRAY1:
				result = translate_from_bits1_to_tgabw(inSource, outDestination,
					bitsHeader.rowBytes, imagespec, brle);
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

// convert a row of uncompressed, non-color mapped
// TGA pixels from ptga to pbits
status_t
pix_tganm_to_bits(uint8 *pbits, uint8 *ptga,
	uint16 width, uint8 depth, uint8 tgaBytesPerPixel,
	uint8 nalpha)
{
	status_t result = B_OK;

	switch (depth) {
		case 32:
			if (nalpha == 8 && tgaBytesPerPixel == 4)
				memcpy(pbits, ptga, 4 * width);
			else if (nalpha == 8) {
				// copy the same 32-bit pixel over and over
				while (width--) {
					memcpy(pbits, ptga, 4);
					pbits += 4;
				}
			} else {
				while (width--) {
					memcpy(pbits, ptga, 3);

					pbits += 4;
					ptga += tgaBytesPerPixel;
				}
			}
			break;

		case 24:
			while (width--) {
				memcpy(pbits, ptga, 3);

				pbits += 4;
				ptga += tgaBytesPerPixel;
			}
			break;

		case 16:
		{
			uint16 val;
			if (nalpha == 1) {
				while (width--) {
					val = ptga[0] + (ptga[1] << 8);
					pbits[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					pbits[1] =
						((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					pbits[2] =
						((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
					pbits[3] = (val & 0x8000) ? 255 : 0;

					pbits += 4;
					ptga += tgaBytesPerPixel;
				}
			} else {
				while (width--) {
					val = ptga[0] + (ptga[1] << 8);
					pbits[0] =
						((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					pbits[1] =
						((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					pbits[2] =
						((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);

					pbits += 4;
					ptga += tgaBytesPerPixel;
				}
			}
			break;
		}

		case 15:
		{
			uint16 val;
			while (width--) {
				val = ptga[0] + (ptga[1] << 8);
				pbits[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				pbits[1] =
					((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				pbits[2] =
					((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);

				pbits += 4;
				ptga += tgaBytesPerPixel;
			}
			break;
		}

		case 8:
			while (width--) {
				memset(pbits, ptga[0], 3);

				pbits += 4;
				ptga += tgaBytesPerPixel;
			}
			break;

		default:
			result = B_ERROR;
			break;
	}

	return result;
}

// ---------------------------------------------------------------
// translate_from_tganm_to_bits
//
// Translates a uncompressed, non-palette TGA from inSource
// to the B_RGB32 or B_RGBA32 bits format.
//
// Preconditions:
//
// Parameters: inSource,	the TGA data to be translated
//
// outDestination,	where the bits data will be written to
//
// filehead, image type info
//
// mapspec, color map info
//
// imagespec, width / height info
//
//
//
// Postconditions:
//
// Returns: B_ERROR, if there is an error allocating memory
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
TGATranslator::translate_from_tganm_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination, TGAFileHeader &filehead,
	TGAColorMapSpec &mapspec, TGAImageSpec &imagespec)
{
	bool bvflip;
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		bvflip = false;
	else
		bvflip = true;
	uint8 nalpha = tga_alphabits(filehead, mapspec, imagespec);
	int32 bitsRowBytes = imagespec.width * 4;
	uint8 tgaBytesPerPixel = (imagespec.depth / 8) +
		((imagespec.depth % 8) ? 1 : 0);
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
	uint8 *tgaRowData = new(std::nothrow) uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new(std::nothrow) uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		tgaRowData = NULL;
		return B_ERROR;
	}

	// perform the actual translation
	memset(bitsRowData, 0xff, bitsRowBytes);
	ssize_t rd = inSource->Read(tgaRowData, tgaRowBytes);
	while (rd == tgaRowBytes) {
		pix_tganm_to_bits(bitsRowData, tgaRowData,
			imagespec.width, imagespec.depth,
			tgaBytesPerPixel, nalpha);

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
	tgaRowData = NULL;
	delete[] bitsRowData;
	bitsRowData = NULL;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_tganmrle_to_bits
//
// Convert non color map, RLE TGA to Be bitmap format
// and write results to outDestination
//
// Preconditions:
//
// Parameters: inSource,	the TGA data to be translated
//
// outDestination,	where the bits data will be written to
//
// filehead, image type info
//
// mapspec, color map info
//
// imagespec, width / height info
//
//
//
// Postconditions:
//
// Returns: B_ERROR, if there is an error allocating memory
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
TGATranslator::translate_from_tganmrle_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination, TGAFileHeader &filehead,
	TGAColorMapSpec &mapspec, TGAImageSpec &imagespec)
{
	status_t result = B_OK;

	bool bvflip;
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		bvflip = false;
	else
		bvflip = true;
	uint8 nalpha = tga_alphabits(filehead, mapspec, imagespec);
	int32 bitsRowBytes = imagespec.width * 4;
	uint8 tgaBytesPerPixel = (imagespec.depth / 8) +
		((imagespec.depth % 8) ? 1 : 0);
	uint16 tgapixrow = 0, tgapixcol = 0;

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
	uint8 *bitsRowData = new(std::nothrow) uint8[bitsRowBytes];
	if (!bitsRowData)
		return B_ERROR;

	// perform the actual translation
	memset(bitsRowData, 0xff, bitsRowBytes);
	uint8 *pbitspixel = bitsRowData;
	uint8 packethead;
	StreamBuffer sbuf(inSource, TGA_STREAM_BUFFER_SIZE);
	ssize_t rd = 0;
	if (sbuf.InitCheck() == B_OK)
		rd = sbuf.Read(&packethead, 1);
	while (rd == 1) {
		// Run Length Packet
		if (packethead & TGA_RLE_PACKET_TYPE_BIT) {
			uint8 tgapixel[4], rlecount;
			rlecount = (packethead & ~TGA_RLE_PACKET_TYPE_BIT) + 1;
			if (tgapixcol + rlecount > imagespec.width) {
				result = B_NO_TRANSLATOR;
				break;
			}
			rd = sbuf.Read(tgapixel, tgaBytesPerPixel);
			if (rd == tgaBytesPerPixel) {
				pix_tganm_to_bits(pbitspixel, tgapixel,
					rlecount, imagespec.depth, 0, nalpha);

				pbitspixel += 4 * rlecount;
				tgapixcol += rlecount;
			} else {
				result = B_NO_TRANSLATOR;
				break; // error
			}

		// Raw Packet
		} else {
			uint8 tgaPixelBuf[512], rawcount;
			uint16 rawbytes;
			rawcount = (packethead & ~TGA_RLE_PACKET_TYPE_BIT) + 1;
			if (tgapixcol + rawcount > imagespec.width) {
				result = B_NO_TRANSLATOR;
				break;
			}
			rawbytes = tgaBytesPerPixel * rawcount;
			rd = sbuf.Read(tgaPixelBuf, rawbytes);
			if (rd == rawbytes) {
				pix_tganm_to_bits(pbitspixel, tgaPixelBuf,
					rawcount, imagespec.depth, tgaBytesPerPixel, nalpha);

				pbitspixel += 4 * rawcount;
				tgapixcol += rawcount;
			} else {
				result = B_NO_TRANSLATOR;
				break;
			}
		}

		if (tgapixcol == imagespec.width) {
			outDestination->Write(bitsRowData, bitsRowBytes);
			tgapixcol = 0;
			tgapixrow++;
			if (tgapixrow == imagespec.height)
				break;
			if (bvflip)
				outDestination->Seek(-(bitsRowBytes * 2), SEEK_CUR);
			pbitspixel = bitsRowData;
		}
		rd = sbuf.Read(&packethead, 1);
	}

	delete[] bitsRowData;
	bitsRowData = NULL;

	return result;
}

// convert a row of color mapped pixels to pbits
status_t
pix_tgam_to_bits(uint8 *pbits, uint8 *ptgaindices,
	uint16 width, uint8 depth, uint8 *pmap)
{
	status_t result = B_OK;
	uint8 *ptgapixel = NULL;

	switch (depth) {
		case 32:
			for (uint16 i = 0; i < width; i++) {
				ptgapixel = pmap +
					(ptgaindices[i] * 4);

				memcpy(pbits, ptgapixel, 4);

				pbits += 4;
			}
			break;

		case 24:
			for (uint16 i = 0; i < width; i++) {
				ptgapixel = pmap +
					(ptgaindices[i] * 3);

				memcpy(pbits, ptgapixel, 3);

				pbits += 4;
			}
			break;

		case 16:
			for (uint16 i = 0; i < width; i++) {
				uint16 val;

				ptgapixel = pmap +
					(ptgaindices[i] * 2);
				val = ptgapixel[0] + (ptgapixel[1] << 8);
				pbits[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				pbits[1] =
					((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				pbits[2] =
					((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
				pbits[3] = (val & 0x8000) ? 255 : 0;

				pbits += 4;
			}
			break;

		case 15:
			for (uint16 i = 0; i < width; i++) {
				uint16 val;

				ptgapixel = pmap +
					(ptgaindices[i] * 2);
				val = ptgapixel[0] + (ptgapixel[1] << 8);
				pbits[0] =
					((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
				pbits[1] =
					((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
				pbits[2] =
					((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);

				pbits += 4;
			}
			break;

		default:
			result = B_ERROR;
			break;
	}

	return result;
}

// ---------------------------------------------------------------
// translate_from_tgam_to_bits
//
// Translates a paletted TGA from inSource to the bits format.
//
// Preconditions:
//
// Parameters: inSource,	the TGA data to be translated
//
// outDestination,	where the bits data will be written to
//
// mapspec, info about the color map (palette)
//
// imagespec, width / height info
//
// pmap, color palette
//
//
// Postconditions:
//
// Returns: B_ERROR, if there is an error allocating memory
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
translate_from_tgam_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination, TGAColorMapSpec &mapspec,
	TGAImageSpec &imagespec, uint8 *pmap)
{
	bool bvflip;
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		bvflip = false;
	else
		bvflip = true;

	int32 bitsRowBytes = imagespec.width * 4;
	uint8 tgaBytesPerPixel = (imagespec.depth / 8) +
		((imagespec.depth % 8) ? 1 : 0);
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
	uint8 *tgaRowData = new(std::nothrow) uint8[tgaRowBytes];
	if (!tgaRowData)
		return B_ERROR;
	uint8 *bitsRowData = new(std::nothrow) uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] tgaRowData;
		tgaRowData = NULL;
		return B_ERROR;
	}

	// perform the actual translation
	memset(bitsRowData, 0xff, bitsRowBytes);
	ssize_t rd = inSource->Read(tgaRowData, tgaRowBytes);
	while (rd == tgaRowBytes) {
		pix_tgam_to_bits(bitsRowData, tgaRowData,
			imagespec.width, mapspec.entrysize, pmap);

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
	tgaRowData = NULL;
	delete[] bitsRowData;
	bitsRowData = NULL;

	return B_OK;
}

// ---------------------------------------------------------------
// translate_from_tgamrle_to_bits
//
// Translates a color mapped or non color mapped RLE TGA from
// inSource to the bits format.
//
// Preconditions:
//
// Parameters: inSource,	the TGA data to be translated
//
// outDestination,	where the bits data will be written to
//
// filehead, image type info
//
// mapspec, info about the color map (palette)
//
// imagespec, width / height info
//
// pmap, color palette
//
//
// Postconditions:
//
// Returns: B_ERROR, if there is an error allocating memory
//
// B_OK, if all went well
// ---------------------------------------------------------------
status_t
TGATranslator::translate_from_tgamrle_to_bits(BPositionIO *inSource,
	BPositionIO *outDestination, TGAFileHeader &filehead,
	TGAColorMapSpec &mapspec, TGAImageSpec &imagespec, uint8 *pmap)
{
	status_t result = B_OK;

	bool bvflip;
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		bvflip = false;
	else
		bvflip = true;
	uint8 nalpha = tga_alphabits(filehead, mapspec, imagespec);
	int32 bitsRowBytes = imagespec.width * 4;
	uint8 tgaPalBytesPerPixel = (mapspec.entrysize / 8) +
		((mapspec.entrysize % 8) ? 1 : 0);
	uint8 tgaBytesPerPixel = (imagespec.depth / 8) +
		((imagespec.depth % 8) ? 1 : 0);
	uint16 tgapixrow = 0, tgapixcol = 0;

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
	uint8 *bitsRowData = new(std::nothrow) uint8[bitsRowBytes];
	if (!bitsRowData)
		return B_ERROR;

	// perform the actual translation
	memset(bitsRowData, 0xff, bitsRowBytes);
	uint8 *pbitspixel = bitsRowData;
	uint8 packethead;
	StreamBuffer sbuf(inSource, TGA_STREAM_BUFFER_SIZE);
	ssize_t rd = 0;
	if (sbuf.InitCheck() == B_OK)
		rd = sbuf.Read(&packethead, 1);
	while (rd == 1) {
		// Run Length Packet
		if (packethead & TGA_RLE_PACKET_TYPE_BIT) {
			uint8 tgaindex, rlecount;
			rlecount = (packethead & ~TGA_RLE_PACKET_TYPE_BIT) + 1;
			if (tgapixcol + rlecount > imagespec.width) {
				result = B_NO_TRANSLATOR;
				break;
			}
			rd = sbuf.Read(&tgaindex, 1);
			if (rd == tgaBytesPerPixel) {
				uint8 *ptgapixel;
				ptgapixel = pmap + (tgaindex * tgaPalBytesPerPixel);

				pix_tganm_to_bits(pbitspixel, ptgapixel, rlecount,
					mapspec.entrysize, 0, nalpha);

				pbitspixel += 4 * rlecount;
				tgapixcol += rlecount;
			} else {
				result = B_NO_TRANSLATOR;
				break; // error
			}

		// Raw Packet
		} else {
			uint8 tgaIndexBuf[128], rawcount;
			rawcount = (packethead & ~TGA_RLE_PACKET_TYPE_BIT) + 1;
			if (tgapixcol + rawcount > imagespec.width) {
				result = B_NO_TRANSLATOR;
				break;
			}
			rd = sbuf.Read(tgaIndexBuf, rawcount);
			if (rd == rawcount) {
				pix_tgam_to_bits(pbitspixel, tgaIndexBuf,
					rawcount, mapspec.entrysize, pmap);

				pbitspixel += 4 * rawcount;
				tgapixcol += rawcount;
			} else {
				result = B_NO_TRANSLATOR;
				break;
			}
		}

		if (tgapixcol == imagespec.width) {
			outDestination->Write(bitsRowData, bitsRowBytes);
			tgapixcol = 0;
			tgapixrow++;
			if (tgapixrow == imagespec.height)
				break;
			if (bvflip)
				outDestination->Seek(-(bitsRowBytes * 2), SEEK_CUR);
			pbitspixel = bitsRowData;
		}
		rd = sbuf.Read(&packethead, 1);
	}

	delete[] bitsRowData;
	bitsRowData = NULL;

	return result;
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
TGATranslator::translate_from_tga(BPositionIO *inSource, uint32 outType,
	BPositionIO *outDestination)
{
	TGAFileHeader fileheader;
	TGAColorMapSpec mapspec;
	TGAImageSpec imagespec;
	bool bheaderonly = false, bdataonly = false;

	status_t result;
	result = identify_tga_header(inSource, NULL, &fileheader, &mapspec,
		&imagespec);
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

		const int32 kbuflen = 1024;
		uint8 buf[kbuflen];
		ssize_t rd = inSource->Read(buf, kbuflen);
		while (rd > 0) {
			outDestination->Write(buf, rd);
			rd = inSource->Read(buf, kbuflen);
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
			ptgapalette = new(std::nothrow) uint8[nentrybytes * mapspec.length];
			inSource->Read(ptgapalette, nentrybytes * mapspec.length);
		}

		bitsHeader.rowBytes = imagespec.width * 4;
		if (fileheader.imagetype != TGA_NOCOMP_BW &&
			fileheader.imagetype != TGA_RLE_BW &&
			tga_alphabits(fileheader, mapspec, imagespec))
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
			case TGA_NOCOMP_BW:
				result = translate_from_tganm_to_bits(inSource,
					outDestination, fileheader, mapspec, imagespec);
				break;

			case TGA_NOCOMP_COLORMAP:
				result = translate_from_tgam_to_bits(inSource,
					outDestination, mapspec, imagespec, ptgapalette);
				break;

			case TGA_RLE_TRUECOLOR:
			case TGA_RLE_BW:
				result = translate_from_tganmrle_to_bits(inSource,
					outDestination, fileheader, mapspec, imagespec);
				break;

			case TGA_RLE_COLORMAP:
				result = translate_from_tgamrle_to_bits(inSource, outDestination,
					fileheader, mapspec, imagespec, ptgapalette);
				break;

			default:
				result = B_NO_TRANSLATOR;
				break;
		}

		delete[] ptgapalette;
		ptgapalette = NULL;

		return result;

	} else
		return B_NO_TRANSLATOR;
}

status_t
TGATranslator::DerivedTranslate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
	BPositionIO *outDestination, int32 baseType)
{
	if (baseType == 1)
		// if inSource is in bits format
		return translate_from_bits(inSource, outType, outDestination);
	else if (baseType == 0)
		// if inSource is NOT in bits format
		return translate_from_tga(inSource, outType, outDestination);
	else
		// if BaseTranslator did not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;
}

BView *
TGATranslator::NewConfigView(TranslatorSettings *settings)
{
	return new(std::nothrow) TGAView(B_TRANSLATE("TGATranslator Settings"),
		B_WILL_DRAW, settings);
}

