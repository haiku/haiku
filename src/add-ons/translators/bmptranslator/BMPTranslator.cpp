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
#include "BMPView.h"

#define min(x,y) ((x < y) ? x : y)
#define max(x,y) ((x > y) ? x : y)

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
		"BMP image"
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
		"BMP image (MS format)"
	}
};

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
	if (header.rowBytes * (header.bounds.Height() + 1) > header.dataSize)
		return B_NO_TRANSLATOR;
			
	if (outInfo) {
		outInfo->type = B_TRANSLATOR_BITMAP;
		outInfo->group = B_TRANSLATOR_BITMAP;
		outInfo->quality = BBT_QUALITY;
		outInfo->capability = BBT_CAPABILITY;
		strcpy(outInfo->name, "Be Bitmap Format (BMPTranslator)");
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

status_t identify_bmp_header(BPositionIO *inSource, translator_info *outInfo,
	ssize_t amtread, uint8 *read, BMPFileHeader *pfileheader = NULL,
	MSInfoHeader *pmsheader = NULL, bool *pfrommsformat = NULL, off_t *os2skip = NULL)
{
	uint8 buf[40];
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
	if (swap_data(B_UINT32_TYPE, ((uint8 *) &fileHeader) + 2, 12,
		B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
		
	uint32 headersize = 0;
	if (inSource->Read(&headersize, 4) != 4)
		return B_NO_TRANSLATOR;
	if (swap_data(B_UINT32_TYPE, &headersize, 4, B_SWAP_LENDIAN_TO_HOST) != B_OK)
		return B_ERROR;
	
	if (headersize == sizeof(MSInfoHeader)) {
		// MS format
		MSInfoHeader msheader;
		msheader.size = headersize;
		if (inSource->Read((uint8 *) (&msheader) + 4, 36) != 36)
			return B_NO_TRANSLATOR;
	
		// convert msheader to host byte order
		if (swap_data(B_UINT32_TYPE, (uint8 *) (&msheader) + 4, 36,
			B_SWAP_LENDIAN_TO_HOST) != B_OK)
			return B_ERROR;
	
		// check if msheader is valid
		if (msheader.planes != 1)
			return B_NO_TRANSLATOR;
		if ((msheader.bitsperpixel != 1 || msheader.compression != BMP_NO_COMPRESS) &&
			(msheader.bitsperpixel != 4 || msheader.compression != BMP_NO_COMPRESS) &&
			(msheader.bitsperpixel != 4 || msheader.compression != BMP_RLE4_COMPRESS) &&
			(msheader.bitsperpixel != 8 || msheader.compression != BMP_NO_COMPRESS) &&
			(msheader.bitsperpixel != 8 || msheader.compression != BMP_RLE8_COMPRESS) &&
			(msheader.bitsperpixel != 24 || msheader.compression != BMP_NO_COMPRESS) &&
			(msheader.bitsperpixel != 32 || msheader.compression != BMP_NO_COMPRESS))
			return B_NO_TRANSLATOR;
		if (msheader.colorsimportant > msheader.colorsused)
			return B_NO_TRANSLATOR;
			
		if (outInfo) {
			outInfo->type = B_BMP_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = BMP_QUALITY;
			outInfo->capability = BMP_CAPABILITY;
			sprintf(outInfo->name, "BMP image (MS format, %d bits", msheader.bitsperpixel);
			if (msheader.compression)
				strcat(outInfo->name, ", RLE)");
			else
				strcat(outInfo->name, ")");
			strcpy(outInfo->MIME, "image/x-bmp");
		}
		
		if (pfileheader)
			(*pfileheader) = fileHeader;
		
		if (pmsheader)
			(*pmsheader) = msheader;
			
		if (pfrommsformat)
			(*pfrommsformat) = true;
		
		return B_OK;

	} else if (headersize == sizeof(OS2InfoHeader)) {
		// OS/2 format
		OS2InfoHeader os2header;
		os2header.size = headersize;
		if (inSource->Read((uint8 *) (&os2header) + 4, 8) != 8)
			return B_NO_TRANSLATOR;
	
		// convert msheader to host byte order
		if (swap_data(B_UINT32_TYPE, (uint8 *) (&os2header) + 4, 8,
			B_SWAP_LENDIAN_TO_HOST) != B_OK)
			return B_ERROR;
	
		// check if msheader is valid
		if (os2header.planes != 1)
			return B_NO_TRANSLATOR;
		if (os2header.bitsperpixel != 1 &&
			os2header.bitsperpixel != 4 &&
			os2header.bitsperpixel != 8 &&
			os2header.bitsperpixel != 24)
			return B_NO_TRANSLATOR;
		
		if (outInfo) {
			outInfo->type = B_BMP_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = BMP_QUALITY;
			outInfo->capability = BMP_CAPABILITY;
			sprintf(outInfo->name, "BMP image (OS/2 format, %d bits)", os2header.bitsperpixel);
			strcpy(outInfo->MIME, "image/x-bmp");
		}
		
		if (pfileheader && pmsheader) {
			pfileheader->magic = 'MB';
			pfileheader->fileSize = 0;
			pfileheader->reserved = 0;
			pfileheader->dataOffset = 0;

			pmsheader->size = 40;
			pmsheader->width = os2header.width;
			pmsheader->height = os2header.height;
			pmsheader->planes = 1;
			pmsheader->bitsperpixel = os2header.bitsperpixel;
			pmsheader->compression = BMP_NO_COMPRESS;
			pmsheader->imagesize = 0;
			pmsheader->xpixperm = 2835; // 72 dpi horizontal
			pmsheader->ypixperm = 2835; // 72 dpi vertical
			pmsheader->colorsused = 0;
			pmsheader->colorsimportant = 0;
			
			int32 padding = 0;
			// determine fileSize / imagesize
			switch (pmsheader->bitsperpixel) {
				case 32:
				case 24:
				{
					if (os2skip && fileHeader.dataOffset > 26)
						(*os2skip) = fileHeader.dataOffset - 26;
						
					uint8 bytesPerPixel = pmsheader->bitsperpixel / 8;
					pfileheader->dataOffset = 54;
					padding = (pmsheader->width * bytesPerPixel) % 4;
					if (padding)
						padding = 4 - padding;
					pmsheader->imagesize = ((pmsheader->width * bytesPerPixel) + padding) *
						pmsheader->height;
					pfileheader->fileSize = pfileheader->dataOffset +
						pmsheader->imagesize;
			
					break;
				}
				
				case 8:
				case 4:
				case 1:
				{
					uint16 ncolors = 1 << pmsheader->bitsperpixel;
					pmsheader->colorsused = ncolors;
					pmsheader->colorsimportant = ncolors;
					if (os2skip && fileHeader.dataOffset > 26 + (ncolors * 3))
						(*os2skip) = fileHeader.dataOffset - (26 + (ncolors * 3));

					uint8 pixelsPerByte = 8 / pmsheader->bitsperpixel;
					pfileheader->dataOffset = 54 + (ncolors * 4);
					if (!(pmsheader->width % pixelsPerByte))
						padding = (pmsheader->width / pixelsPerByte) % 4;
					else
						padding = ((pmsheader->width + pixelsPerByte - 
							(pmsheader->width % pixelsPerByte)) / pixelsPerByte) % 4;
					if (padding)
						padding = 4 - padding;
					pmsheader->imagesize = ((pmsheader->width / pixelsPerByte) + ((pmsheader->width % pixelsPerByte) ? 1 : 0) + padding) *
						pmsheader->height;
					pfileheader->fileSize = pfileheader->dataOffset +
						pmsheader->imagesize;
					
					break;
				}
				
				default:
					break;
			}
		}
		
		if (pfrommsformat)
			(*pfrommsformat) = false;
		
		return B_OK;

	} else
		return B_NO_TRANSLATOR;
}

status_t BMPTranslator::Identify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != B_BMP_FORMAT)
		return B_NO_TRANSLATOR;

	uint8 ch[4];
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

// for converting uncompressed BMP images with no palette to the Be Bitmap format
status_t translate_from_bits_to_bmp24(BPositionIO *inSource, BPositionIO *outDestination,
	color_space fromspace, MSInfoHeader &msheader)
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
	int32 bitsRowBytes = msheader.width * bitsBytesPerPixel;
	int32 bmpBytesPerPixel = msheader.bitsperpixel / 8;
	int32 padding = (msheader.width * bmpBytesPerPixel) % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = (msheader.width * bmpBytesPerPixel) + padding;
	uint32 bmppixrow = 0;
	off_t bitsoffset = ((msheader.height - 1) * bitsRowBytes);
	inSource->Seek(bitsoffset, SEEK_CUR);
	uint8 *bmpRowData = new uint8[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	for (int32 i = 1; i <= padding; i++)
		bmpRowData[bmpRowBytes - i] = 0;
	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	const color_map *pmap = system_colors();
	while (rd == bitsRowBytes) {
	
		for (uint32 i = 0; i < msheader.width; i++) {
			uint8 *bitspixel, *bmppixel;
			uint16 val;
			switch (fromspace) {
				case B_RGB32:
				case B_RGBA32:
				case B_RGB24:
					memcpy(bmpRowData + (i * 3),
						bitsRowData + (i * bitsBytesPerPixel), 3);
					break;
					
				case B_RGB16:
				case B_RGB16_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					if (fromspace == B_RGB16)
						val = bitspixel[0] + (bitspixel[1] << 8);
					else
						val = bitspixel[1] + (bitspixel[1] << 8);
					bmppixel[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					bmppixel[1] = ((val & 0x7e0) >> 3) | ((val & 0x7e0) >> 9);
					bmppixel[2] = ((val & 0xf800) >> 8) | ((val & 0xf800) >> 13);
					break;

				case B_RGB15:
				case B_RGB15_BIG:
				case B_RGBA15:
				case B_RGBA15_BIG:
					// NOTE: the alpha data for B_RGBA15* is ignored and discarded
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					if (fromspace == B_RGB15 || fromspace == B_RGBA15)
						val = bitspixel[0] + (bitspixel[1] << 8);
					else
						val = bitspixel[1] + (bitspixel[0] << 8);
					bmppixel[0] = ((val & 0x1f) << 3) | ((val & 0x1f) >> 2);
					bmppixel[1] = ((val & 0x3e0) >> 2) | ((val & 0x3e0) >> 7);
					bmppixel[2] = ((val & 0x7c00) >> 7) | ((val & 0x7c00) >> 12);
					break;
						
				case B_RGB32_BIG:
				case B_RGBA32_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					bmppixel[0] = bitspixel[3];
					bmppixel[1] = bitspixel[2];
					bmppixel[2] = bitspixel[1];
					break;
						
				case B_RGB24_BIG:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					bmppixel[0] = bitspixel[2];
					bmppixel[1] = bitspixel[1];
					bmppixel[2] = bitspixel[0];
					break;
				
				case B_CMAP8:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					rgb_color c = pmap->color_list[bitspixel[0]];
					bmppixel[0] = c.blue;
					bmppixel[1] = c.green;
					bmppixel[2] = c.red;
					break;
					
				case B_GRAY8:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					bmppixel[0] = bitspixel[0];
					bmppixel[1] = bitspixel[0];
					bmppixel[2] = bitspixel[0];
					break;
					
				case B_CMYK32:
				{
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					
					int32 comp = 255 - bitspixel[2] - bitspixel[3];
					bmppixel[0] = (comp < 0) ? 0 : comp;
					
					comp = 255 - bitspixel[1] - bitspixel[3];
					bmppixel[1] = (comp < 0) ? 0 : comp;
					
					comp = 255 - bitspixel[0] - bitspixel[3];
					bmppixel[2] = (comp < 0) ? 0 : comp;
					break;
				}
				
				case B_CMY32:
				case B_CMYA32:
				case B_CMY24:
					bitspixel = bitsRowData + (i * bitsBytesPerPixel);
					bmppixel = bmpRowData + (i * 3);
					bmppixel[0] = 255 - bitspixel[2];
					bmppixel[1] = 255 - bitspixel[1];
					bmppixel[2] = 255 - bitspixel[0];
					break;
						
				default:
					break;
			} // switch (fromspace)
		} // for for (uint32 i = 0; i < msheader.width; i++)
				
		outDestination->Write(bmpRowData, bmpRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == msheader.height)
			break;

		inSource->Seek(-(bitsRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

// for converting uncompressed BMP images with no palette to the Be Bitmap format
status_t translate_from_bits8_to_bmp8(BPositionIO *inSource, BPositionIO *outDestination,
	int32 bitsRowBytes, MSInfoHeader &msheader)
{
	int32 padding;
	padding = msheader.width % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = msheader.width + padding;
	uint32 bmppixrow = 0;
	off_t bitsoffset = ((msheader.height - 1) * bitsRowBytes);
	inSource->Seek(bitsoffset, SEEK_CUR);
	uint8 *bmpRowData = new uint8[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	while (rd == bitsRowBytes) {
		memcpy(bmpRowData, bitsRowData, msheader.width);
		if (padding) {
			uint8 zeros[] = {0, 0, 0, 0};
			memcpy(bmpRowData + msheader.width, zeros, padding);
		}
		outDestination->Write(bmpRowData, bmpRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == msheader.height)
			break;

		inSource->Seek(-(bitsRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

// for converting uncompressed BMP images with no palette to the Be Bitmap format
status_t translate_from_bits1_to_bmp1(BPositionIO *inSource, BPositionIO *outDestination,
	int32 bitsRowBytes, MSInfoHeader &msheader)
{
	uint16 pixelsPerByte = 8 / msheader.bitsperpixel;
	int32 padding;
	if (!(msheader.width % pixelsPerByte))
		padding = (msheader.width / pixelsPerByte) % 4;
	else
		padding = ((msheader.width + pixelsPerByte - 
			(msheader.width % pixelsPerByte)) / pixelsPerByte) % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = (msheader.width / pixelsPerByte) + 
		((msheader.width % pixelsPerByte) ? 1 : 0) + padding;
	uint32 bmppixrow = 0;
	off_t bitsoffset = ((msheader.height - 1) * bitsRowBytes);
	inSource->Seek(bitsoffset, SEEK_CUR);
	uint8 *bmpRowData = new uint8[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	ssize_t rd = inSource->Read(bitsRowData, bitsRowBytes);
	while (rd == bitsRowBytes) {
		uint32 bmppixcol = 0;
		for (int32 i = 0; i < bmpRowBytes; i++)
			bmpRowData[i] = 0;
		for (int32 i = 0; (bmppixcol < msheader.width) && (i < bitsRowBytes); i++) {
			// process each byte in the row
			uint8 pixels = bitsRowData[i];
			for (uint8 compbit = 128; (bmppixcol < msheader.width) && compbit; compbit >>= 1) {
				// for each bit in the current byte, convert to a BMP palette index and
				// store that in the bmpRowData
				uint8 index;
				if (pixels & compbit)
					// 1 == black
					index = 1;
				else
					// 0 == white
					index = 0;
				bmpRowData[bmppixcol / pixelsPerByte] |= index << (7 - (bmppixcol % pixelsPerByte));
				bmppixcol++;
			}
		}
				
		outDestination->Write(bmpRowData, bmpRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == msheader.height)
			break;

		inSource->Seek(-(bitsRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bitsRowData, bitsRowBytes);
	} // while (rd == bitsRowBytes)
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

status_t write_bmp_headers(BPositionIO *outDestination, BMPFileHeader &fileHeader,
	MSInfoHeader &msheader)
{
	uint8 bmpheaders[54];
	memcpy(bmpheaders, &fileHeader.magic, sizeof(uint16));
	memcpy(bmpheaders + 2, &fileHeader.fileSize, sizeof(uint32));
	memcpy(bmpheaders + 6, &fileHeader.reserved, sizeof(uint32));
	memcpy(bmpheaders + 10, &fileHeader.dataOffset, sizeof(uint32));
	memcpy(bmpheaders + 14, &msheader, sizeof(msheader));
	if (swap_data(B_UINT16_TYPE, bmpheaders, 2,
		B_SWAP_HOST_TO_LENDIAN) != B_OK)
		return B_ERROR;
	if (swap_data(B_UINT32_TYPE, bmpheaders + 2, 12,
		B_SWAP_HOST_TO_LENDIAN) != B_OK)
		return B_ERROR;
	if (swap_data(B_UINT32_TYPE, bmpheaders + 14,
		sizeof(MSInfoHeader), B_SWAP_HOST_TO_LENDIAN) != B_OK)
		return B_ERROR;
	if (outDestination->Write(bmpheaders, 54) != 54)
		return B_ERROR;
		
	return B_OK;
}

status_t translate_from_bits(BPositionIO *inSource, ssize_t amtread,
	uint8 *read, bool bheaderonly, bool bdataonly, uint32 outType, BPositionIO *outDestination)
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
			if (swap_data(B_UINT32_TYPE, &bitsHeader, sizeof(TranslatorBitmap),
				B_SWAP_HOST_TO_BENDIAN) != B_OK)
				return B_ERROR;
			if (outDestination->Write(&bitsHeader,
				sizeof(TranslatorBitmap)) != sizeof(TranslatorBitmap))
				return B_ERROR;
		}
		
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
		
	// Translate B_TRANSLATOR_BITMAP to B_BMP_FORMAT
	} else if (outType == B_BMP_FORMAT) {
		// Set up BMP header
		BMPFileHeader fileHeader;
		fileHeader.magic = 'MB';
		fileHeader.reserved = 0;
		
		MSInfoHeader msheader;
		msheader.size = 40;
		msheader.width = (uint32) bitsHeader.bounds.Width() + 1;
		msheader.height = (uint32) bitsHeader.bounds.Height() + 1;
		msheader.planes = 1;
		msheader.xpixperm = 2835; // 72 dpi horizontal
		msheader.ypixperm = 2835; // 72 dpi vertical
		msheader.colorsused = 0;
		msheader.colorsimportant = 0;
		
		int32 padding = 0;
		// determine fileSize / imagesize
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
	
				fileHeader.dataOffset = 54;
				msheader.bitsperpixel = 24;
				msheader.compression = BMP_NO_COMPRESS;
				padding = (msheader.width * 3) % 4;
				if (padding)
					padding = 4 - padding;
				msheader.imagesize = ((msheader.width * 3) + padding) *
					msheader.height;
				fileHeader.fileSize = fileHeader.dataOffset +
					msheader.imagesize;
			
				break;
				
			case B_CMAP8:
			case B_GRAY8:
			
				msheader.colorsused = 256;
				msheader.colorsimportant = 256;
				fileHeader.dataOffset = 54 + (4 * 256);
				msheader.bitsperpixel = 8;
				msheader.compression = BMP_NO_COMPRESS;
				padding = msheader.width % 4;
				if (padding)
					padding = 4 - padding;
				msheader.imagesize = (msheader.width + padding) * msheader.height;
				fileHeader.fileSize = fileHeader.dataOffset +
					msheader.imagesize;
					
				break;
				
			case B_GRAY1:
			
				msheader.colorsused = 2;
				msheader.colorsimportant = 2;
				fileHeader.dataOffset = 62;
				msheader.bitsperpixel = 1;
				msheader.compression = BMP_NO_COMPRESS;
				if (!(msheader.width % 8))
					padding = (msheader.width / 8) % 4;
				else
					padding = ((msheader.width + 8 - 
						(msheader.width % 8)) / 8) % 4;
				if (padding)
					padding = 4 - padding;
				msheader.imagesize = ((msheader.width / 8) + ((msheader.width % 8) ? 1 : 0) + padding) *
					msheader.height;
				fileHeader.fileSize = fileHeader.dataOffset +
					msheader.imagesize;
					
				break;
				
			default:
				return B_NO_TRANSLATOR;
		}
		
		// write out the BMP headers
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			result = write_bmp_headers(outDestination, fileHeader, msheader);
			if (result != B_OK)
				return result;
		}
		if (bheaderonly)
			// if user only wants the header, bail out
			// before the data is written
			return result;

		// write out the BMP pixel data
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
				return translate_from_bits_to_bmp24(inSource, outDestination,
					bitsHeader.colors, msheader);
					
			case B_CMAP8:
			{
				uint8 pal[1024] = { 0 };
				const color_map *pmap = system_colors();
				for (int32 i = 0; i < 256; i++) {
					uint8 *palent = pal + (i * 4);
					rgb_color c = pmap->color_list[i];
					palent[0] = c.blue;
					palent[1] = c.green;
					palent[2] = c.red;
				}
				if (outDestination->Write(pal, 1024) != 1024)
					return B_ERROR;

				return translate_from_bits8_to_bmp8(inSource, outDestination,
					bitsHeader.rowBytes, msheader);
			}
			
			case B_GRAY8:
			{
				uint8 pal[1024] = { 0 };
				for (int32 i = 0; i < 256; i++) {
					uint8 *palent = pal + (i * 4);
					palent[0] = i;
					palent[1] = i;
					palent[2] = i;
				}
				if (outDestination->Write(pal, 1024) != 1024)
					return B_ERROR;

				return translate_from_bits8_to_bmp8(inSource, outDestination,
					bitsHeader.rowBytes, msheader);
			}
				
			case B_GRAY1:
			{
				const uint32 monopal[] = { 0x00ffffff, 0x00000000 };
				if (outDestination->Write(monopal, 8) != 8)
					return B_ERROR;
				
				return translate_from_bits1_to_bmp1(inSource, outDestination,
					bitsHeader.rowBytes, msheader);
			}
				
			default:
				break;
		}
		
		return B_NO_TRANSLATOR;
	} else
		return B_NO_TRANSLATOR;
}

// for converting uncompressed BMP images with no palette to the Be Bitmap format
status_t crappy_translate_from_bmpnpal_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, MSInfoHeader &msheader)
{
	int32 bitsRowBytes = msheader.width * 4;
	int32 bmpBytesPerPixel = msheader.bitsperpixel / 8;
	int32 padding = (msheader.width * bmpBytesPerPixel) % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = (msheader.width * bmpBytesPerPixel) + padding;
	uint8 *imageData = new uint8[max(bmpRowBytes * msheader.height, bitsRowBytes * msheader.height)];
		// I just tried to allocate an awful lot of memory
	if (!imageData)
		return B_ERROR;
	ssize_t rd = inSource->Read(imageData, bmpRowBytes * msheader.height);
	if (rd != bmpRowBytes * msheader.height)
		return B_NO_TRANSLATOR;
	uint32 pixelsleft = msheader.width * msheader.height;
	uint32 bmprow = msheader.height - 1;
	while (pixelsleft > 0) {
		uint32 destindex, srcindex;
		destindex = (pixelsleft * 4) - 1;
		srcindex = (bmpRowBytes * bmprow) + (((pixelsleft - 1) % msheader.width) * bmpBytesPerPixel) + (bmpBytesPerPixel - 1);
		if (bmpBytesPerPixel == 3)
			imageData[destindex--] = 0;
		for (int32 i = 0; i < bmpBytesPerPixel; i++)
			imageData[destindex--] = imageData[srcindex--];
		
		pixelsleft--;
		if (!(pixelsleft % msheader.width))
			bmprow--;
	}
	
	// write out bits data
	uint8 *bitsRow = new uint8[bitsRowBytes];
	if (!bitsRow) {
		delete[] imageData;
		return B_ERROR;
	}
	for (uint32 i = 0; i < msheader.height / 2; i++) {
		uint8 *frontrow, *backrow;
		frontrow = imageData + (i * bitsRowBytes);
		backrow = imageData + ((msheader.height - i - 1) * bitsRowBytes);
		
		memcpy(bitsRow, frontrow, bitsRowBytes);
		memcpy(frontrow, backrow, bitsRowBytes);
		memcpy(backrow, bitsRow, bitsRowBytes);
	}
	delete[] bitsRow;
	outDestination->Write(imageData, bitsRowBytes * msheader.height);
	delete[] imageData;

	return B_OK;
}

// for converting uncompressed BMP images with no palette to the Be Bitmap format
status_t translate_from_bmpnpal_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, MSInfoHeader &msheader)
{
	int32 bitsRowBytes = msheader.width * 4;
	int32 bmpBytesPerPixel = msheader.bitsperpixel / 8;
	int32 padding = (msheader.width * bmpBytesPerPixel) % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = (msheader.width * bmpBytesPerPixel) + padding;
	uint32 bmppixrow = 0;
	uint8 *bmpRowData = new uint8[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	ssize_t rd = inSource->Read(bmpRowData, bmpRowBytes);
	while (rd == bmpRowBytes) {
		uint8 *pBitsPixel = bitsRowData;
		uint8 *pBmpPixel = bmpRowData;
		for (uint32 i = 0; i < msheader.width; i++) {
			memcpy(pBitsPixel, pBmpPixel, 3);
			pBitsPixel += 4;
			pBmpPixel += bmpBytesPerPixel;
		}
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == msheader.height)
			break;

		rd = inSource->Read(bmpRowData, bmpRowBytes);
	}
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

// for converting uncompressed BMP images with no palette to the Be Bitmap format
status_t mediocre_translate_from_bmpnpal_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, MSInfoHeader &msheader)
{
	int32 bitsRowBytes = msheader.width * 4;
	int32 bmpBytesPerPixel = msheader.bitsperpixel / 8;
	int32 padding = (msheader.width * bmpBytesPerPixel) % 4;
	if (padding)
		padding = 4 - padding;
	int32 bmpRowBytes = (msheader.width * bmpBytesPerPixel) + padding;
	uint32 bmppixrow = 0;
	off_t bmpoffset = ((msheader.height - 1) * bmpRowBytes);
	inSource->Seek(bmpoffset, SEEK_CUR);
	uint8 *bmpRowData = new uint8[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	for (int32 i = 1; i <= padding; i++)
		bitsRowData[bitsRowBytes - i] = 0;
	ssize_t rd = inSource->Read(bmpRowData, bmpRowBytes);
	while (rd == bmpRowBytes) {
		uint8 *pBitsPixel = bitsRowData;
		uint8 *pBmpPixel = bmpRowData;
		for (uint32 i = 0; i < msheader.width; i++) {
			memcpy(pBitsPixel, pBmpPixel, 3);
			pBitsPixel += 4;
			pBmpPixel += bmpBytesPerPixel;
		}
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == msheader.height)
			break;

		inSource->Seek(-(bmpRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bmpRowData, bmpRowBytes);
	}
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

// for converting palette-based uncompressed BMP images to the Be Bitmap format
status_t translate_from_bmppal_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, MSInfoHeader &msheader, const uint8 *palette, bool frommsformat)
{
	uint16 pixelsPerByte = 8 / msheader.bitsperpixel;
	uint16 bitsPerPixel = msheader.bitsperpixel;
	uint8 palBytesPerPixel;
	if (frommsformat)
		palBytesPerPixel = 4;
	else
		palBytesPerPixel = 3;
	
	uint8 mask = 1;
	mask = (mask << bitsPerPixel) - 1;
	
	int32 padding;
	if (!(msheader.width % pixelsPerByte))
		padding = (msheader.width / pixelsPerByte) % 4;
	else
		padding = ((msheader.width + pixelsPerByte - 
			(msheader.width % pixelsPerByte)) / pixelsPerByte) % 4;
	if (padding)
		padding = 4 - padding;

	int32 bmpRowBytes = (msheader.width / pixelsPerByte) + 
		((msheader.width % pixelsPerByte) ? 1 : 0) + padding;
	uint32 bmppixrow = 0;

	off_t bmpoffset = ((msheader.height - 1) * bmpRowBytes);
	inSource->Seek(bmpoffset, SEEK_CUR);
	uint8 *bmpRowData = new uint8[bmpRowBytes];
	if (!bmpRowData)
		return B_ERROR;
	int32 bitsRowBytes = msheader.width * 4;
	uint8 *bitsRowData = new uint8[bitsRowBytes];
	if (!bitsRowData) {
		delete[] bmpRowData;
		return B_ERROR;
	}
	for (int32 i = 3; i < bitsRowBytes; i += 4)
		bitsRowData[i] = 0xff;
	ssize_t rd = inSource->Read(bmpRowData, bmpRowBytes);
	while (rd == bmpRowBytes) {
		for (uint32 i = 0; i < msheader.width; i++) {
			uint8 indices = (bmpRowData + (i / pixelsPerByte))[0];
			uint8 index;
			index = (indices >>
				(bitsPerPixel * ((pixelsPerByte - 1) - (i % pixelsPerByte)))) & mask;
			memcpy(bitsRowData + (i * 4),
				palette + (index * palBytesPerPixel), 3);
		}
				
		outDestination->Write(bitsRowData, bitsRowBytes);
		bmppixrow++;
		// if I've read all of the pixel data, break
		// out of the loop so I don't try to read 
		// non-pixel data
		if (bmppixrow == msheader.height)
			break;

		inSource->Seek(-(bmpRowBytes * 2), SEEK_CUR);
		rd = inSource->Read(bmpRowData, bmpRowBytes);
	}
	
	delete[] bmpRowData;
	delete[] bitsRowData;

	return B_OK;
}

// for converting palette-based RLE BMP images to the Be Bitmap format
status_t translate_from_bmppalr_to_bits(BPositionIO *inSource, BPositionIO *outDestination,
	int32 datasize, MSInfoHeader &msheader, const uint8 *palette)
{
	uint16 pixelsPerByte = 8 / msheader.bitsperpixel;
	uint16 bitsPerPixel = msheader.bitsperpixel;
	uint8 mask = (1 << bitsPerPixel) - 1;

	uint8 count, indices, index;
	uint8 *bitspixels = new uint8[datasize];
	if (!bitspixels)
		return B_ERROR;
	int32 bitsRowBytes = msheader.width * 4;
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
				bitsoffset = ((msheader.height -
					(bmppixrow + 1)) * bitsRowBytes) +
						(bmppixcol * 4);
				index = (indices >> (bitsPerPixel * ((pixelsPerByte - 1) - (i % pixelsPerByte)))) & mask;
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
					while (bmppixcol != msheader.width) {
						bitsoffset = ((msheader.height -
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
					if (bmppixcol == msheader.width) {
						bmppixcol = 0;
						bmppixrow++;
					}
					
					while (bmppixrow < msheader.height) {
						bitsoffset = ((msheader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
						
						if (bmppixcol == msheader.width) {
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
						bitsoffset = ((msheader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
									
						if (bmppixcol == msheader.width) {
							bmppixcol = 0;
							bmppixrow++;
							dy--;
						}
					}
								
					// get to the same column as where we started
					while (x != bmppixcol) {
						bitsoffset = ((msheader.height -
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						memcpy(bitspixels + bitsoffset, palette, 3);
						bmppixcol++;
					}
								
					// move over dx pixels
					for (uint8 i = 0; i < dx; i++) {
						bitsoffset = ((msheader.height -
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
					// read code uncompressed indices
					uint8 uncomp[256];
					int32 padding;
					if (!(code % pixelsPerByte))
						padding = (code / pixelsPerByte) % 2;
					else
						padding = ((code + pixelsPerByte - 
							(code % pixelsPerByte)) / pixelsPerByte) % 2;
					int32 uncompBytes = (code / pixelsPerByte) + 
						((code % pixelsPerByte) ? 1 : 0) + padding;
					rd = inSource->Read(uncomp, uncompBytes);
					if (rd != uncompBytes) {
						rd = 0;
						break;
					}
					for (uint8 i = 0; i < code; i++) {
						bitsoffset = ((msheader.height - 
							(bmppixrow + 1)) *
								bitsRowBytes) +
								(bmppixcol * 4);
						indices = (uncomp + (i / pixelsPerByte))[0];
						index = (indices >>
							(bitsPerPixel * ((pixelsPerByte - 1) - (i % pixelsPerByte)))) & mask;
						memcpy(bitspixels + bitsoffset,
							palette + (index * 4), 3);
						bmppixcol++;
					}

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

status_t translate_from_bmp(BPositionIO *inSource, ssize_t amtread,
	uint8 *read, bool bheaderonly, bool bdataonly, uint32 outType, BPositionIO *outDestination)
{
	BMPFileHeader fileHeader;
	MSInfoHeader msheader;
	bool frommsformat;
	off_t os2skip = 0;

	status_t result;
	result = identify_bmp_header(inSource, NULL, amtread, read, &fileHeader, &msheader, &frommsformat, &os2skip);
	if (result != B_OK)
		return result;
	
	// if the user wants to translate a BMP to a BMP, easy enough :)	
	if (outType == B_BMP_FORMAT) {
		// write out the BMP headers
		if (bheaderonly || (!bheaderonly && !bdataonly)) {
			result = write_bmp_headers(outDestination, fileHeader, msheader);
			if (result != B_OK)
				return result;
		}
		if (bheaderonly)
			// if the user only wants the header, 
			// bail before it is written
			return result;
			
		uint8 buf[1024];
		ssize_t rd;
		if (!frommsformat && (msheader.bitsperpixel == 1 ||	msheader.bitsperpixel == 4 || msheader.bitsperpixel == 8)) {
			// if OS/2 paletted format, convert palette to MS format
			uint16 ncolors = 1 << msheader.bitsperpixel;
			rd = inSource->Read(buf, ncolors * 3);
			if (rd != ncolors * 3)
				return B_NO_TRANSLATOR;
			uint8 mspalent[4] = {0, 0, 0, 0};
			for (uint16 i = 0; i < ncolors; i++) {
				memcpy(mspalent, buf + (i * 3), 3);
				outDestination->Write(mspalent, 4);
			}
		}
		// if there is junk between the OS/2 headers and
		// the actual data, skip it
		if (!frommsformat && os2skip)
			inSource->Seek(os2skip, SEEK_CUR);

		uint32 rdtotal = fileHeader.dataOffset;
		rd = min(1024, fileHeader.fileSize - rdtotal);
		rd = inSource->Read(buf, rd);
		while (rd > 0) {
			outDestination->Write(buf, rd);
			rdtotal += rd;
			rd = min(1024, fileHeader.fileSize - rdtotal);
			rd = inSource->Read(buf, rd);
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
		bitsHeader.bounds.right = msheader.width - 1;
		bitsHeader.bounds.bottom = msheader.height - 1;
		
		// read in palette and/or skip non-BMP data					
		uint8 bmppalette[1024];
		off_t nskip = 0;
		if (msheader.bitsperpixel == 1 ||
			msheader.bitsperpixel == 4 ||
			msheader.bitsperpixel == 8) {
			
			uint8 palBytesPerPixel;
			if (frommsformat)
				palBytesPerPixel = 4;
			else
				palBytesPerPixel = 3;
			
			if (!msheader.colorsused) {
				msheader.colorsused = 1;
				msheader.colorsused <<= msheader.bitsperpixel;
			}
			
			if (inSource->Read(bmppalette, msheader.colorsused * palBytesPerPixel) !=
				msheader.colorsused * palBytesPerPixel)
				return B_NO_TRANSLATOR;
				
			// skip over non-BMP data
			if (frommsformat) {
				if (fileHeader.dataOffset > (msheader.colorsused * palBytesPerPixel) + 54)
					nskip = fileHeader.dataOffset - ((msheader.colorsused * palBytesPerPixel) + 54);
			} else
				nskip = os2skip;
		} else if (fileHeader.dataOffset > 54)
			// skip over non-BMP data
			nskip = fileHeader.dataOffset - 54;
			
		if (nskip > 0 && inSource->Seek(nskip, SEEK_CUR) < 0)
			return B_NO_TRANSLATOR;

		bitsHeader.rowBytes = msheader.width * 4;
		bitsHeader.colors = B_RGB32;
		int32 datasize = bitsHeader.rowBytes * msheader.height;
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
		
		switch (msheader.bitsperpixel) {
			case 32:
			case 24:
				return translate_from_bmpnpal_to_bits(inSource, outDestination,
					datasize, msheader);
				
			case 8:
				// 8 bit BMP with NO compression
				if (msheader.compression == BMP_NO_COMPRESS)
					return translate_from_bmppal_to_bits(inSource,
						outDestination, datasize, msheader, bmppalette, frommsformat);

				// 8 bit RLE compressed BMP
				else if (msheader.compression == BMP_RLE8_COMPRESS)
					return translate_from_bmppalr_to_bits(inSource,
						outDestination, datasize, msheader, bmppalette);
				else
					return B_NO_TRANSLATOR;
			
			case 4:
				// 4 bit BMP with NO compression
				if (!msheader.compression)	
					return translate_from_bmppal_to_bits(inSource,
						outDestination, datasize, msheader, bmppalette, frommsformat);

				// 4 bit RLE compressed BMP
				else if (msheader.compression == BMP_RLE4_COMPRESS)
					return translate_from_bmppalr_to_bits(inSource,
						outDestination, datasize, msheader, bmppalette);
				else
					return B_NO_TRANSLATOR;
			
			case 1:
				return translate_from_bmppal_to_bits(inSource,
					outDestination, datasize, msheader, bmppalette, frommsformat);
					
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
	
	uint8 ch[4];
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
	uint16 n16ch;
	memcpy(&n16ch, ch, sizeof(uint16));
	// if B_TRANSLATOR_BITMAP type	
	if (n32ch == nbits)
		return translate_from_bits(inSource, 4, ch, bheaderonly, bdataonly, outType, outDestination);
	// if BMP type in Little Endian byte order
	else if (n16ch == nbm)
		return translate_from_bmp(inSource, 4, ch, bheaderonly, bdataonly, outType, outDestination);
	else
		return B_NO_TRANSLATOR;
}

status_t BMPTranslator::MakeConfigurationView(BMessage *ioExtension,
	BView **outView, BRect *outExtent)
{
	if (!outView || !outExtent)
		return B_BAD_VALUE;

	BMPView *view = new BMPView(BRect(0,0,225,175),
		"BMPTranslator Settings", B_FOLLOW_ALL, B_WILL_DRAW);
	*outView = view;
	*outExtent = view->Bounds();

	return B_OK;
}
