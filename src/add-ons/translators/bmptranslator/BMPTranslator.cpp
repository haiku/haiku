/*****************************************************************************/
// [Application Name]
//
// Version: [0.0.0] [Development Stage]
//
// [Description]
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
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
#include "BMPTranslator.h"

#define min(x,y) ((x < y) ? x : y)

translation_format gInputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_QUALITY,
		BBT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (BMPTranslator)"
	},
	{
		B_BMP_FORMAT,
		B_TRANSLATOR_BITMAP,
		BMP_QUALITY,
		BMP_CAPABILITY,
		"image/x-bmp",
		"BMP image, MS format"
	}
};

translation_format gOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_QUALITY,
		BBT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (BMPTranslator)"
	},
	{
		B_BMP_FORMAT,
		B_TRANSLATOR_BITMAP,
		BMP_QUALITY,
		BMP_CAPABILITY,
		"image/x-bmp",
		"BMP image, MS format"
	}
};

int main(int argc, char **argv)
{
	return 0;
}

BTranslator *make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new BMPTranslator();
	else
		return NULL;
}

BMPTranslator::BMPTranslator() : BTranslator()
{
	strcpy(fName, "BMP Images");
	sprintf(fInfo, "BMP image translator v%d.%d.%d %s\n",
		BMP_TRANSLATOR_VERSION / 100, (BMP_TRANSLATOR_VERSION / 10) % 10,
		BMP_TRANSLATOR_VERSION % 10, __DATE__);
}

BMPTranslator::~BMPTranslator()
{
}
	
const char *BMPTranslator::TranslatorName() const
{
	return fName;
}

const char *BMPTranslator::TranslatorInfo() const
{
	return fInfo;
}

int32 BMPTranslator::TranslatorVersion() const
{
	return BMP_TRANSLATOR_VERSION;
}

const translation_format *BMPTranslator::InputFormats(int32 *out_count) const
{
	if (out_count)
		*out_count = 2;

	return gInputFormats;
}
		
const translation_format *BMPTranslator::OutputFormats(int32 *out_count) const
{
	if (out_count)
		*out_count = 2;

	return gOutputFormats;
}

status_t identify_bits_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, char *read)
{
	TranslatorBitmap header;
		
	memcpy(&header, read, amtread);
		// copy portion of header already read in
	// read in the rest of the header
	ssize_t size = sizeof(TranslatorBitmap) - amtread;
	if (inSource->Read(((char *) &header) + amtread, size) != size)
		return B_NO_TRANSLATOR;
		
	// convert to host byte order
	if (swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
		
	// check if header values are reasonable
	if (header.colors != B_RGB32 &&
		header.colors != B_RGBA32 &&
		header.colors != B_RGB24 &&
		header.colors != B_RGB16 &&
		header.colors != B_RGB15 &&
		header.colors != B_RGBA15 &&
		header.colors != B_CMAP8 &&
		header.colors != B_GRAY8 &&
		header.colors != B_GRAY1)
		return B_NO_TRANSLATOR;
	if (header.rowBytes * (header.bounds.Height() + 1) > header.dataSize)
		return B_NO_TRANSLATOR;
			
	outInfo->type = B_TRANSLATOR_BITMAP;
	outInfo->group = B_TRANSLATOR_BITMAP;
	outInfo->quality = BBT_QUALITY;
	outInfo->capability = BBT_CAPABILITY;
	strcpy(outInfo->name, "Be Bitmap Format (BMPTranslator)");
	strcpy(outInfo->MIME, "image/x-be-bitmap");
	
	return B_OK;
}

status_t identify_bmp_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, char *read)
{
	char buf[40];
	BMPFileHeader fileHeader;
	
	memcpy(buf, read, amtread);
		// copy portion of fileHeader already read in
	// read in the rest of the fileHeader
	ssize_t size = 14 - amtread;
	if (inSource->Read(buf + amtread, size) != size)
		return B_NO_TRANSLATOR;
	
	// convert fileHeader to host byte order
	memcpy(&fileHeader.magic, buf, 2);
	memcpy(&fileHeader.fileSize, buf + 2, 4);
	memcpy(&fileHeader.reserved, buf + 6, 4);
	memcpy(&fileHeader.dataOffset, buf + 10, 4);
	if (swap_data(B_UINT16_TYPE, &fileHeader.magic, sizeof(uint16),
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	if (swap_data(B_UINT32_TYPE, ((char *) &fileHeader) + 2, 12,
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	
	BMPInfoHeader infoHeader;
	size = sizeof(BMPInfoHeader);
	if (inSource->Read(&infoHeader, size) != size)
		return B_NO_TRANSLATOR;
	
	// convert infoHeader to host byte order
	if (swap_data(B_UINT32_TYPE, &infoHeader, size,
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	
	// check if infoHeader is valid
	if (infoHeader.size != sizeof(BMPInfoHeader))
		return B_NO_TRANSLATOR;
	if (infoHeader.planes != 1)
		return B_NO_TRANSLATOR;
	if ((infoHeader.bitsperpixel != 1 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 4 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 4 || infoHeader.compression != BMP_RLE4_COMPRESS) &&
		(infoHeader.bitsperpixel != 8 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 8 || infoHeader.compression != BMP_RLE8_COMPRESS) &&
		(infoHeader.bitsperpixel != 24 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 32 || infoHeader.compression != BMP_NO_COMPRESS))
		return B_NO_TRANSLATOR;
	if (infoHeader.colorsimportant > infoHeader.colorsused)
		return B_NO_TRANSLATOR;
		
	outInfo->type = B_BMP_FORMAT;
	outInfo->group = B_TRANSLATOR_BITMAP;
	outInfo->quality = BMP_QUALITY;
	outInfo->capability = BMP_CAPABILITY;
	strcpy(outInfo->name, "BMP image, MS format");
	strcpy(outInfo->MIME, "image/x-bmp");
	
	return B_OK;
}

status_t BMPTranslator::Identify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_BMP_FORMAT)
		return B_NO_TRANSLATOR;

	char ch[4];
	uint32 nbits = B_TRANSLATOR_BITMAP;
	uint16 nbm = 'MB';
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	if (swap_data(B_UINT32_TYPE, &nbits, 4, B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	if (swap_data(B_UINT16_TYPE, &nbm, 2, B_SWAP_HOST_TO_LENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	uint16 n16ch;
	memcpy(&n16ch, ch, sizeof(uint16));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return identify_bits_header(inSource, outInfo, 4, ch);
	// if BMP type in Little Endian byte order
	else if (n16ch == nbm)
		return identify_bmp_header(inSource, outInfo, 4, ch);
	else
		return B_NO_TRANSLATOR;
}

status_t translate_from_bits(BPositionIO *inSource, ssize_t amtread,
	char *read, uint32 outType, BPositionIO *outDestination)
{
	TranslatorBitmap bitsHeader;
		
	memcpy(&bitsHeader, read, amtread);
		// copy portion of bitsHeader already read in
	// read in the rest of the bitsHeader
	ssize_t size = sizeof(TranslatorBitmap) - amtread;
	if (inSource->Read(((char *) &bitsHeader) + amtread, size) != size)
		return B_NO_TRANSLATOR;
		
	// convert to host byte order
	if (swap_data(B_UINT32_TYPE, &bitsHeader, sizeof(TranslatorBitmap),
		B_SWAP_BENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
		
	// check if bitsHeader values are reasonable
	if (bitsHeader.colors != B_RGB32 &&
		bitsHeader.colors != B_RGBA32 &&
		bitsHeader.colors != B_RGB24 &&
		bitsHeader.colors != B_RGB16 &&
		bitsHeader.colors != B_RGB15 &&
		bitsHeader.colors != B_RGBA15 &&
		bitsHeader.colors != B_CMAP8 &&
		bitsHeader.colors != B_GRAY8 &&
		bitsHeader.colors != B_GRAY1)
		return B_NO_TRANSLATOR;
	if (bitsHeader.rowBytes * (bitsHeader.bounds.Height() + 1) >
		bitsHeader.dataSize)
		return B_NO_TRANSLATOR;
	
	// Translate B_TRANSLATOR_BITMAP to B_TRANSLATOR_BITMAP, easy enough :)	
	if (outType == B_TRANSLATOR_BITMAP) {
		// write out bitsHeader
		if (swap_data(B_UINT32_TYPE, &bitsHeader, sizeof(TranslatorBitmap),
			B_SWAP_HOST_TO_BENDIAN) != B_OK)
			return B_ERROR;
		if (outDestination->Write(&bitsHeader,
			sizeof(TranslatorBitmap)) != sizeof(TranslatorBitmap))
			return B_ERROR;
			
		char buf[1024];
		ssize_t rd = inSource->Read(buf, 1024);
		while (rd > 0) {
			outDestination->Write(buf, rd);
			rd = inSource->Read(buf, 1024);
		}
		
		if (rd == 0)
			return B_OK;
		else
			return B_ERROR;
		
	// Translate B_TRANSLATOR_BITMAP to B_BMP_FORMAT
	} else if (outType == B_BMP_FORMAT) {
		// Set up BMP header
		BMPFileHeader fileHeader;
		fileHeader.magic = 'MB';
		fileHeader.reserved = 0;
		fileHeader.dataOffset = 54;
		
		BMPInfoHeader infoHeader;
		infoHeader.size = 40;
		infoHeader.width = (uint32) bitsHeader.bounds.Width() + 1;
		infoHeader.height = (uint32) bitsHeader.bounds.Height() + 1;
		infoHeader.planes = 1;
		infoHeader.xpixperm = 2835; // 72 dpi horizontal
		infoHeader.ypixperm = 2835; // 72 dpi vertical
		infoHeader.colorsused = 0;
		infoHeader.colorsimportant = 0;
		
		int32 padding = 0;
		// determine fileSize / imagesize
		switch (bitsHeader.colors) {
			case B_RGB32:
			case B_RGBA32:
			case B_RGB24:
			
				infoHeader.bitsperpixel = 24;
				infoHeader.compression = BMP_NO_COMPRESS;
				padding = (infoHeader.width * 3) % 4;
				if (padding)
					padding = 4 - padding;
				infoHeader.imagesize = ((infoHeader.width * 3) + padding) *
					infoHeader.height;
				fileHeader.fileSize = fileHeader.dataOffset +
					infoHeader.imagesize;
			
				break;
				
			default:
				return B_NO_TRANSLATOR;
		}
		
		// write out the BMP headers
		char bmpheaders[54];
		memcpy(bmpheaders, &fileHeader.magic, sizeof(uint16));
		memcpy(bmpheaders + 2, &fileHeader.fileSize, sizeof(uint32));
		memcpy(bmpheaders + 6, &fileHeader.reserved, sizeof(uint32));
		memcpy(bmpheaders + 10, &fileHeader.dataOffset, sizeof(uint32));
		memcpy(bmpheaders + 14, &infoHeader, sizeof(infoHeader));
		if (swap_data(B_UINT16_TYPE, bmpheaders, 2,
			B_SWAP_HOST_TO_LENDIAN) != B_OK)
			return B_ERROR;
		if (swap_data(B_UINT32_TYPE, bmpheaders + 2, 12,
			B_SWAP_HOST_TO_LENDIAN) != B_OK)
			return B_ERROR;
		if (swap_data(B_UINT32_TYPE, bmpheaders + 14,
			sizeof(BMPInfoHeader), B_SWAP_HOST_TO_LENDIAN) != B_OK)
			return B_ERROR;
		if (outDestination->Write(bmpheaders, 54) != 54)
			return B_ERROR;

		// write out the BMP pixel data
		char *bmppixels = new char[infoHeader.imagesize];
		if (!bmppixels)
			return B_ERROR;
			
		switch (bitsHeader.colors) {
			case B_RGB32:
				char bitspixel[4];
				int32 bytesperline = (infoHeader.width * 3) + padding;
				int32 bitspixoffset = 0;
				ssize_t bitsoffset = 0;
				ssize_t rd = inSource->Read(bitspixel, 4);
				while (rd > 0) {
					int32 bmpoffset = ((infoHeader.height -
						((bitsoffset / bitsHeader.rowBytes) + 1)) *
							bytesperline) + (bitspixoffset * 3);
					memcpy(bmppixels + bmpoffset, bitspixel, 3);
					
					bitspixoffset++;
					bitsoffset += rd;
					if (!(bitsoffset % bitsHeader.rowBytes)) {
						bitspixoffset = 0;
						// put padding at the end of the line
						if (padding) {
							char chpadding[4] = { 0, 0, 0, 0 };
							memcpy(bmppixels + bmpoffset + 3, chpadding,
								padding);
						}
						
						// after I've read the entire bitmap, make sure to bail
						if (bitsoffset / bitsHeader.rowBytes ==
							infoHeader.height)
							break;
					}
					rd = inSource->Read(bitspixel, 4);
				}
				outDestination->Write(bmppixels, infoHeader.imagesize);
				break;
		}	
		
		delete[] bmppixels;
		
		return B_OK;
	} else
		return B_NO_TRANSLATOR;
}

status_t translate_from_bmpnpal_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, BMPInfoHeader &infoHeader)
{
	int32 bitsRowBytes = infoHeader.width * 4;
	int32 bmpBytesPerPixel = infoHeader.bitsperpixel / 8;
	int32 padding = (infoHeader.width * bmpBytesPerPixel) % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = (infoHeader.width * bmpBytesPerPixel) + padding;
	uint32 bmppixrow = 0;
	off_t bmpoffset = ((infoHeader.height - 1) * bmpRowBytes);
	inSource->Seek(bmpoffset, SEEK_CUR);
	char *bmpRowData = new char[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	char *bitsRowData = new char[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	memset(bitsRowData, 0xffffffff, bitsRowBytes);
	ssize_t rd = inSource->Read(bmpRowData, bmpRowBytes);
	while (rd == bmpRowBytes) {
		for (uint32 i = 0; i < infoHeader.width; i++)
			memcpy(bitsRowData + (i * 4),
				bmpRowData + (i * bmpBytesPerPixel), 3);
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == infoHeader.height)
			break;

		inSource->Seek(-(bmpRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bmpRowData, bmpRowBytes);
	}
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

status_t translate_from_bmp8r_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, BMPInfoHeader &infoHeader, const char *palette)
{
	uint8 count, index;
	char *bitspixels = new char[datasize];
	if (!bitspixels)
		return B_ERROR;
	memset(bitspixels, 0xffffffff, datasize);

	int32 bitsRowBytes = infoHeader.width * 4;
	uint32 bmppixcol = 0, bmppixrow = 0;
	int32 bitsoffset = 0;
	ssize_t rd = inSource->Read(&count, 1);
	while (rd > 0) {
		// repeated color
		if (count) {
			rd = inSource->Read(&index, 1);
			if (rd != 1)
				break;
			for (uint8 i = 0; i < count; i++) {
				bitsoffset = ((infoHeader.height -
					(bmppixrow + 1)) * bitsRowBytes) +
						(bmppixcol * 4);
				memcpy(bitspixels + bitsoffset, palette + (index * 4), 3);
				bmppixcol++;
			}
		// special code
		} else {
			uint8 code;
			rd = inSource->Read(&code, 1);
			if (rd != 1)
				break;
			switch (code) {
				case 0:
					// end of line
					// if there are columns remaing on this
					// line, set them to the color at index zero
					while (bmppixcol != infoHeader.width) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
					bmppixcol = 0;
					bmppixrow++;
					break;
								
				case 1:
					// end of bitmap
					if (bmppixcol == infoHeader.width) {
						bmppixcol = 0;
						bmppixrow++;
					}
					
					while (bmppixrow < infoHeader.height) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
						
						if (bmppixcol == infoHeader.width) {
							bmppixcol = 0;
							bmppixrow++;
						}
					}
					rd = 0;
						// break out of while loop
					break;
								
				case 2:
				{
					// delta, wierd feature
					uint8 x, dx, dy;
					inSource->Read(&dx, 1);
					inSource->Read(&dy, 1);
								
					x = bmppixcol;
								
					// set all pixels to the first entry in
					// the palette, for the number of rows skipped
					while (dy > 0) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
									
						if (bmppixcol == infoHeader.width) {
							bmppixcol = 0;
							bmppixrow++;
							dy--;
						}
					}
								
					// get to the same column as where we started
					while (x != bmppixcol) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
								
					// move over dx pixels
					for (uint8 i = 0; i < dx; i++) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
					
					break;
				}
								
				// code >= 3
				default:
					// read code uncompressed pixels
					for (uint8 i = 0; i < code; i++) {
						rd = inSource->Read(&index, 1);
						if (rd != 1)
							break;
						bitsoffset = ((infoHeader.height - 
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset,
							palette + (index * 4), 3);
						bmppixcol++;
					}
					// if odd number, there is junk padding
					if (code % 2)
						rd = inSource->Seek(1, SEEK_CUR);
					break;
			}
		}
		if (rd > 0)
			rd = inSource->Read(&count, 1);
	}
	outDestination->Write(bitspixels, datasize);
				
	delete[] bitspixels;
	return B_OK;
}

status_t translate_from_bmp4r_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, BMPInfoHeader &infoHeader, const char *palette)
{
	uint8 count, indices, index;
	char *bitspixels = new char[datasize];
	if (!bitspixels)
		return B_ERROR;
	memset(bitspixels, 0xffffffff, datasize);

	int32 bitsRowBytes = infoHeader.width * 4;
	uint32 bmppixcol = 0, bmppixrow = 0;
	int32 bitsoffset = 0;
	ssize_t rd = inSource->Read(&count, 1);
	while (rd > 0) {
		// repeated color
		if (count) {
			rd = inSource->Read(&indices, 1);
			if (rd != 1)
				break;
			for (uint8 i = 0; i < count; i++) {
				bitsoffset = ((infoHeader.height -
					(bmppixrow + 1)) * bitsRowBytes) +
						(bmppixcol * 4);
				if (!(i % 2))
					index = (indices>>4) & 0xf;
				else
					index = indices & 0xf;
				memcpy(bitspixels + bitsoffset, palette + (index * 4), 3);
				bmppixcol++;
			}
		// special code
		} else {
			uint8 code;
			rd = inSource->Read(&code, 1);
			if (rd != 1)
				break;
			switch (code) {
				case 0:
					// end of line
					// if there are columns remaing on this
					// line, set them to the color at index zero
					while (bmppixcol != infoHeader.width) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
					bmppixcol = 0;
					bmppixrow++;
					break;
								
				case 1:
					// end of bitmap
					if (bmppixcol == infoHeader.width) {
						bmppixcol = 0;
						bmppixrow++;
					}
					
					while (bmppixrow < infoHeader.height) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
						
						if (bmppixcol == infoHeader.width) {
							bmppixcol = 0;
							bmppixrow++;
						}
					}
					rd = 0;
						// break out of while loop
					break;
								
				case 2:
				{
					// delta, wierd feature
					uint8 x, dx, dy;
					inSource->Read(&dx, 1);
					inSource->Read(&dy, 1);
								
					x = bmppixcol;
								
					// set all pixels to the first entry in
					// the palette, for the number of rows skipped
					while (dy > 0) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
									
						if (bmppixcol == infoHeader.width) {
							bmppixcol = 0;
							bmppixrow++;
							dy--;
						}
					}
								
					// get to the same column as where we started
					while (x != bmppixcol) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
								
					// move over dx pixels
					for (uint8 i = 0; i < dx; i++) {
						bitsoffset = ((infoHeader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
					
					break;
				}
							
							
				// code >= 3
				default:
					// read code uncompressed pixels
					for (uint8 i = 0; i < code; i++) {
						if (!(i % 2)) {
							rd = inSource->Read(&indices, 1);
							if (rd != 1)
								break;
						}
						bitsoffset = ((infoHeader.height - 
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						if (!(i % 2))
							index = (indices>>4) & 0xf;
						else
							index = indices & 0xf;
						memcpy(bitspixels + bitsoffset,
							palette + (index * 4), 3);
						bmppixcol++;
					}
								
					int32 padding;
					padding = (((code + (code % 2)) / 2) % 2);
					// if odd number, there is junk padding
					if (padding)
						inSource->Seek(padding, SEEK_CUR);
					break;
			}
		}
		if (rd > 0)
			rd = inSource->Read(&count, 1);
	}
	outDestination->Write(bitspixels, datasize);
				
	delete[] bitspixels;
	return B_OK;
}

status_t translate_from_bmppal_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, BMPInfoHeader &infoHeader, const char *palette)
{
	uint16 pixelsPerByte = 8 / infoHeader.bitsperpixel;
	uint16 bitsPerPixel = infoHeader.bitsperpixel;
	
	uint8 mask = 1;
	for (uint16 i = 0; i < bitsPerPixel; i++)
		mask *= 2;
	mask -= 1;
	
	int32 padding;
	if (!(infoHeader.width % pixelsPerByte))
		padding = (infoHeader.width / pixelsPerByte) % 4;
	else
		padding = ((infoHeader.width + pixelsPerByte - 
			(infoHeader.width % pixelsPerByte)) / pixelsPerByte) % 4;
	if (padding)
		padding = 4 - padding;

	int32 bmpRowBytes = (infoHeader.width / pixelsPerByte) + 
		((infoHeader.width % pixelsPerByte) ? 1 : 0) + padding;
	uint32 bmppixrow = 0;

	off_t bmpoffset = ((infoHeader.height - 1) * bmpRowBytes);
	inSource->Seek(bmpoffset, SEEK_CUR);
	char *bmpRowData = new char[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	int32 bitsRowBytes = infoHeader.width * 4;
	char *bitsRowData = new char[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	memset(bitsRowData, 0xffffffff, bitsRowBytes);
	ssize_t rd = inSource->Read(bmpRowData, bmpRowBytes);
	while (rd == bmpRowBytes) {
		for (uint32 i = 0; i < infoHeader.width; i++) {
			uint8 indices = (bmpRowData + (i / pixelsPerByte))[0];
			uint8 index;
			index = (indices >>
				(bitsPerPixel * ((pixelsPerByte - 1) - (i % pixelsPerByte)))) & mask;
			memcpy(bitsRowData + (i * 4),
				palette + (4 * index), 3);
		}
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == infoHeader.height)
			break;

		inSource->Seek(-(bmpRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bmpRowData, bmpRowBytes);
	}
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

status_t translate_from_bmp(BPositionIO *inSource, ssize_t amtread,
	char *read, uint32 outType, BPositionIO *outDestination)
{
	char buf[40];
	BMPFileHeader fileHeader;
	
	memcpy(buf, read, amtread);
		// copy portion of fileHeader already read in
	// read in the rest of the fileHeader
	ssize_t size = 14 - amtread;
	if (inSource->Read(buf + amtread, size) != size)
		return B_NO_TRANSLATOR;
	
	// convert fileHeader to host byte order
	memcpy(&fileHeader.magic, buf, 2);
	memcpy(&fileHeader.fileSize, buf + 2, 4);
	memcpy(&fileHeader.reserved, buf + 6, 4);
	memcpy(&fileHeader.dataOffset, buf + 10, 4);
	if (swap_data(B_UINT16_TYPE, &fileHeader.magic, 2,
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	if (swap_data(B_UINT32_TYPE, ((char *) &fileHeader) + 2, 12,
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	
	BMPInfoHeader infoHeader;
	size = sizeof(BMPInfoHeader);
	if (inSource->Read(&infoHeader, size) != size)
		return B_NO_TRANSLATOR;
	
	// convert infoHeader to host byte order
	if (swap_data(B_UINT32_TYPE, &infoHeader, size,
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	
	// check if infoHeader is valid
	if (infoHeader.size != sizeof(BMPInfoHeader))
		return B_NO_TRANSLATOR;
	if (infoHeader.planes != 1)
		return B_NO_TRANSLATOR;
	if ((infoHeader.bitsperpixel != 1 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 4 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 4 || infoHeader.compression != BMP_RLE4_COMPRESS) &&
		(infoHeader.bitsperpixel != 8 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 8 || infoHeader.compression != BMP_RLE8_COMPRESS) &&
		(infoHeader.bitsperpixel != 24 || infoHeader.compression != BMP_NO_COMPRESS) &&
		(infoHeader.bitsperpixel != 32 || infoHeader.compression != BMP_NO_COMPRESS))
		return B_NO_TRANSLATOR;
	if (infoHeader.colorsimportant > infoHeader.colorsused)
		return B_NO_TRANSLATOR;
	
	// if the user wants to translate a BMP to a BMP, easy enough :)	
	if (outType == B_BMP_FORMAT) {
		// write out fileHeader
		if (swap_data(B_UINT16_TYPE, &fileHeader.magic, sizeof(uint16),
			B_SWAP_HOST_TO_LENDIAN) != B_OK)
			return B_ERROR;
		if (swap_data(B_UINT32_TYPE, ((char *) &fileHeader) + 2, 12,
			B_SWAP_HOST_TO_LENDIAN) != B_OK)
			return B_ERROR;
		if (outDestination->Write(&fileHeader.magic, 2) != 2)
			return B_ERROR;
		if (outDestination->Write(((char *) &fileHeader) + 2, 12) != 12)
			return B_ERROR;
	
		// write out infoHeader
		if (swap_data(B_UINT32_TYPE, &infoHeader, sizeof(BMPInfoHeader),
			B_SWAP_HOST_TO_LENDIAN) != B_OK)
			return B_ERROR;
		if (outDestination->Write(&infoHeader, sizeof(BMPInfoHeader)) !=
			sizeof(BMPInfoHeader))
			return B_ERROR;
			
		char buf[1024];
		ssize_t rd = inSource->Read(buf, 1024);
		while (rd > 0) {
			outDestination->Write(buf, rd);
			rd = inSource->Read(buf, 1024);
		}
		
		if (rd == 0)
			return B_OK;
		else
			return B_ERROR;
	
	// if translating a BMP to a Be Bitmap
	} else if (outType == B_TRANSLATOR_BITMAP) {
		TranslatorBitmap bitsHeader;
		bitsHeader.magic = B_TRANSLATOR_BITMAP;
		bitsHeader.bounds.left = 0;
		bitsHeader.bounds.top = 0;
		bitsHeader.bounds.right = infoHeader.width - 1;
		bitsHeader.bounds.bottom = infoHeader.height - 1;
		
		// read in palette and/or skip non-BMP data					
		char bmppalette[1024];
		off_t nskip = 0;
		if (infoHeader.bitsperpixel == 1 ||
			infoHeader.bitsperpixel == 4 ||
			infoHeader.bitsperpixel == 8) {
			
			if (!infoHeader.colorsused) {
				infoHeader.colorsused = 1;
				for (int npow = 0; npow < infoHeader.bitsperpixel; npow++)
					infoHeader.colorsused *= 2;
			}
			
			if (inSource->Read(bmppalette, infoHeader.colorsused * 4) !=
				infoHeader.colorsused * 4)
				return B_NO_TRANSLATOR;
				
			// skip over non-BMP data
			if (fileHeader.dataOffset > (infoHeader.colorsused * 4) + 54)
				nskip = fileHeader.dataOffset - ((infoHeader.colorsused * 4) + 54);
		} else if (fileHeader.dataOffset > 54)
			// skip over non-BMP data
			nskip = fileHeader.dataOffset - 54;
			
		if (nskip > 0 && inSource->Seek(nskip, SEEK_CUR) < 0)
			return B_NO_TRANSLATOR;

		bitsHeader.rowBytes = infoHeader.width * 4;
		bitsHeader.colors = B_RGB32;
		int32 datasize = bitsHeader.rowBytes * infoHeader.height;
		bitsHeader.dataSize = datasize;
				
		// write out Be's Bitmap header
		if (swap_data(B_UINT32_TYPE, &bitsHeader,
			sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK)
			return B_ERROR;
		outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
		
		switch (infoHeader.bitsperpixel) {
			case 32:
			case 24:
				return translate_from_bmpnpal_to_bits(inSource, outDestination,
					datasize, infoHeader);
				
			case 8:
				// 8 bit BMP with NO compression
				if (infoHeader.compression == BMP_NO_COMPRESS)
					return translate_from_bmppal_to_bits(inSource,
						outDestination, datasize, infoHeader, bmppalette);

				// 8 bit RLE compressed BMP
				else if (infoHeader.compression == BMP_RLE8_COMPRESS)
					return translate_from_bmp8r_to_bits(inSource,
						outDestination, datasize, infoHeader, bmppalette);
				else
					return B_NO_TRANSLATOR;
			
			case 4:
				// 4 bit BMP with NO compression
				if (!infoHeader.compression)	
					return translate_from_bmppal_to_bits(inSource,
						outDestination, datasize, infoHeader, bmppalette);

				// 4 bit RLE compressed BMP
				else if (infoHeader.compression == BMP_RLE4_COMPRESS)
					return translate_from_bmp4r_to_bits(inSource,
						outDestination, datasize, infoHeader, bmppalette);
				else
					return B_NO_TRANSLATOR;
			
			case 1:
				return translate_from_bmppal_to_bits(inSource,
					outDestination, datasize, infoHeader, bmppalette);
					
			default:
				return B_NO_TRANSLATOR;
		}
		
	} else
		return B_NO_TRANSLATOR;
}

status_t BMPTranslator::Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_BMP_FORMAT)
		return B_NO_TRANSLATOR;
		
	inSource->Seek(0, SEEK_SET);
	
		char ch[4];
	uint32 nbits = B_TRANSLATOR_BITMAP;
	uint16 nbm = 'MB';
	
	// Convert the magic numbers to the various byte orders so that
	// I won't have to convert the data read in to see whether or not
	// it is a supported type
	if (swap_data(B_UINT32_TYPE, &nbits, sizeof(uint32),
		B_SWAP_HOST_TO_BENDIAN) != B_OK)
		return B_ERROR;
	if (swap_data(B_UINT16_TYPE, &nbm, sizeof(uint16),
		B_SWAP_HOST_TO_LENDIAN) != B_OK)
		return B_ERROR;
	
	// Read in the magic number and determine if it
	// is a supported type
	if (inSource->Read(ch, 4) != 4)
		return B_NO_TRANSLATOR;
	
	uint32 n32ch;
	memcpy(&n32ch, ch, sizeof(uint32));
	uint16 n16ch;
	memcpy(&n16ch, ch, sizeof(uint16));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return translate_from_bits(inSource, 4, ch, outType, outDestination);
	// if BMP type in Little Endian byte order
	else if (n16ch == nbm)
		return translate_from_bmp(inSource, 4, ch, outType, outDestination);
	else
		return B_NO_TRANSLATOR;
}
