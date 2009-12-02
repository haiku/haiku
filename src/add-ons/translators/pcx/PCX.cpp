/*
 * Copyright 2008, Jérôme Duval, korli@users.berlios.de. All rights reserved.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "PCX.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ByteOrder.h>

#include "StreamBuffer.h"
#include "PCXTranslator.h"


//#define TRACE_PCX
#ifdef TRACE_PCX
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


using namespace PCX;


class TempAllocator {
	public:
		TempAllocator() : fMemory(NULL) {}
		~TempAllocator() { free(fMemory); }

		void *Allocate(size_t size) { return fMemory = malloc(size); }

	private:
		void	*fMemory;
};


bool
pcx_header::IsValid() const
{
	TRACE("manufacturer:%u version:%u encoding:%u bitsPerPixel:%u numPlanes:%u bytesPerLine:%u\n", manufacturer, version, encoding, bitsPerPixel, numPlanes, bytesPerLine);
	return manufacturer == 10
		&& version == 5
		&& encoding == 1
		&& (bitsPerPixel == 1 || bitsPerPixel == 4 || bitsPerPixel == 8)
		&& (numPlanes == 1 || numPlanes == 3)
		&& (bitsPerPixel == 8 || numPlanes == 1)
		&& (bytesPerLine & 1) == 0;
}


void
pcx_header::SwapToHost()
{
	swap_data(B_UINT16_TYPE, this, sizeof(pcx_header), B_SWAP_LENDIAN_TO_HOST);
}


void
pcx_header::SwapFromHost()
{
	swap_data(B_UINT16_TYPE, this, sizeof(pcx_header), B_SWAP_HOST_TO_LENDIAN);
}


//	#pragma mark -


static status_t
convert_data_to_bits(pcx_header &header, StreamBuffer &source,
	BPositionIO &target)
{
	uint16 bitsPerPixel = header.bitsPerPixel;
	uint16 bytesPerLine = header.bytesPerLine;
	uint32 width = header.xMax - header.xMin + 1;
	uint32 height = header.yMax - header.yMin + 1;
	uint16 numPlanes = header.numPlanes;
	uint32 scanLineLength = numPlanes * bytesPerLine;

	// allocate buffers
	TempAllocator scanLineAllocator;
	TempAllocator paletteAllocator;
	uint8 *scanLineData[height];
	uint8 *palette = (uint8 *)paletteAllocator.Allocate(3 * 256);
	status_t status = B_OK;

	for (uint32 row = 0; row < height; row++) {
		TRACE("scanline %ld\n", row);
		scanLineData[row] = (uint8 *)scanLineAllocator.Allocate(scanLineLength);
		if (scanLineData[row] == NULL)
			return B_NO_MEMORY;
		uint8 *line = scanLineData[row];
		uint32 index = 0;
		uint8 x;
		do {
			if (source.Read(&x, 1) != 1) {
				status = B_IO_ERROR;
				break;
			}
			if ((x & 0xc0) == 0xc0) {
				uint32 count = x & 0x3f;
				if (index + count - 1 > scanLineLength) {
					status = B_BAD_DATA;
					break;
				}
				if (source.Read(&x, 1) != 1) {
					status = B_IO_ERROR;
					break;
				}
				for (uint32 i = 0; i < count; i++)
					line[index++] = x;
			} else {
				line[index++] = x;
			}
		} while (index < scanLineLength);

		if (status != B_OK) {
			// If we've already read more than a third of the file, display
			// what we have, ie. ignore the error.
			if (row < height / 3)
				return status;

			memset(scanLineData + row, 0, sizeof(uint8*) * (height - row));
			break;
		}
	}

	if (bitsPerPixel == 8 && numPlanes == 1) {
		TRACE("palette reading %p 8\n", palette);
		uint8 x;
		if (status != B_OK || source.Read(&x, 1) != 1 || x != 12) {
			// Try again by repositioning the file stream
			if (source.Seek(-3 * 256 - 1, SEEK_END) < 0)
				return B_BAD_DATA;
			if (source.Read(&x, 1) != 1)
				return B_IO_ERROR;
			if (x != 12)
				return B_BAD_DATA;
		}
		if (source.Read(palette, 256 * 3) != 256 * 3)
			return B_IO_ERROR;
	} else {
		TRACE("palette reading %p palette\n", palette);
		memcpy(palette, &header.paletteInfo, 48);
	}

	uint8 alpha = 255;
	if (bitsPerPixel == 1 && numPlanes == 1) {
		TRACE("target writing 1\n");
		palette[0] = palette[1] = palette[2] = 0;
		palette[3] = palette[4] = palette[5] = 0xff;
		for (uint32 row = 0; row < height; row++) {
			uint8 *line = scanLineData[row];
			if (line == NULL)
				break;
			uint8 mask[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
			for (uint32 i = 0; i < width; i++) {
				bool isBit = ((line[i >> 3] & mask[i & 7]) != 0) ? true : false;
				target.Write(&palette[!isBit ? 2 : 5], 1);
				target.Write(&palette[!isBit ? 1 : 4], 1);
				target.Write(&palette[!isBit ? 0 : 3], 1);
				target.Write(&alpha, 1);
			}
		}
	} else if (bitsPerPixel == 4 && numPlanes == 1) {
		TRACE("target writing 4\n");
		for (uint32 row = 0; row < height; row++) {
			uint8 *line = scanLineData[row];
			if (line == NULL)
				break;
			for (uint32 i = 0; i < width; i++) {
				uint16 index;
				if ((i & 1) == 0)
					index = (line[i >> 1] >> 4) & 15;
				else
					index = line[i >> 1] & 15;
				TRACE("target writing 4 i %d index %d\n", i, index);
				index += (index + index);
				target.Write(&palette[index+2], 1);
				target.Write(&palette[index+1], 1);
				target.Write(&palette[index], 1);
				target.Write(&alpha, 1);
			}
		}
	} else if (bitsPerPixel == 8 && numPlanes == 1) {
		TRACE("target writing 8\n");
		for (uint32 row = 0; row < height; row++) {
			TRACE("target writing 8 row %ld\n", row);
			uint8 *line = scanLineData[row];
			if (line == NULL)
				break;
			for (uint32 i = 0; i < width; i++) {
				uint16 index = line[i];
				index += (index + index);
				target.Write(&palette[index+2], 1);
				target.Write(&palette[index+1], 1);
				target.Write(&palette[index], 1);
				target.Write(&alpha, 1);
			}

		}
	} else {
		TRACE("target writing raw\n");
		for (uint32 row = 0; row < height; row++) {
			uint8 *line = scanLineData[row];
			if (line == NULL)
				break;
			for (uint32 i = 0; i < width; i++) {
				target.Write(&line[i + 2 * bytesPerLine], 1);
				target.Write(&line[i + bytesPerLine], 1);
				target.Write(&line[i], 1);
				target.Write(&alpha, 1);
			}
		}
	}

	return B_OK;
}


//	#pragma mark -


status_t
PCX::identify(BMessage *settings, BPositionIO &stream, uint8 &type, int32 &bitsPerPixel)
{
	// read in the header

	pcx_header header;
	if (stream.Read(&header, sizeof(pcx_header)) != (ssize_t)sizeof(pcx_header))
		return B_BAD_VALUE;

	header.SwapToHost();

	// check header

	if (!header.IsValid())
		return B_BAD_VALUE;

	bitsPerPixel = header.bitsPerPixel;

	TRACE("PCX::identify OK\n");

	return B_OK;
}


/**	Converts an PCX image of any type into a B_RGBA32 B_TRANSLATOR_BITMAP.
 */

status_t
PCX::convert_pcx_to_bits(BMessage *settings, BPositionIO &source, BPositionIO &target)
{
	StreamBuffer sourceBuf(&source, 2048);
	if (sourceBuf.InitCheck() != B_OK)
		return B_IO_ERROR;

	pcx_header header;
	if (sourceBuf.Read(&header, sizeof(pcx_header)) != (ssize_t)sizeof(pcx_header))
		return B_BAD_VALUE;

	header.SwapToHost();

	// check header

	if (!header.IsValid())
		return B_BAD_VALUE;

	uint16 width = header.xMax - header.xMin + 1;
	uint16 height = header.yMax - header.yMin + 1;

	TranslatorBitmap bitsHeader;
	bitsHeader.magic = B_TRANSLATOR_BITMAP;
	bitsHeader.bounds.left = 0;
	bitsHeader.bounds.top = 0;
	bitsHeader.bounds.right = width - 1;
	bitsHeader.bounds.bottom = height - 1;
	bitsHeader.bounds.Set(0, 0, width - 1, height - 1);
	bitsHeader.rowBytes = width * 4;
	bitsHeader.colors = B_RGB32;
	bitsHeader.dataSize = bitsHeader.rowBytes * height;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &bitsHeader, sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN);
	target.Write(&bitsHeader, sizeof(TranslatorBitmap));

	return convert_data_to_bits(header, sourceBuf, target);
}
