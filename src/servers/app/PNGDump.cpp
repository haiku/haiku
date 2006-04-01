/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	Function for saving a generic framebuffer to a PNG file */

#include "PNGDump.h"

#include <NodeInfo.h>
#include <Rect.h>

#include <png.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define TRACE_PNGDUMP
#ifdef TRACE_PNGDUMP
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


status_t
SaveToPNG(const char* filename, const BRect& bounds, color_space space,
	const void* bits, int32 bitsLength, int32 bytesPerRow)
{
	int32 width = bounds.IntegerWidth() + 1;
	int32 height = bounds.IntegerHeight() + 1;

	TRACE(("SaveToPNG: %s (%ldx%ld)\n", filename, width, height));

	FILE *file = fopen(filename, "wb");
	if (file == NULL) {
		TRACE(("Couldn't open file: %s\n", strerror(errno)));
		return errno;
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (png == NULL) {
		TRACE(("Couldn't create write struct\n"));
		fclose(file);
		return B_NO_MEMORY;
	}

	png_infop info = png_create_info_struct(png);
	if (info == NULL) {
		TRACE(("Couldn't create info struct\n"));
		png_destroy_write_struct(&png, NULL);
		fclose(file);
		return B_NO_MEMORY;
	}

	if (setjmp(png->jmpbuf)) {
		png_destroy_write_struct(&png, NULL);
		fclose(file);
		return B_ERROR;
	}

	png_init_io(png, file);
	png_set_bgr(png);

	// TODO: support other color spaces if needed

	switch (space) {
		case B_RGB32:
		case B_RGBA32:
		{
			// create file without alpha channel
			png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
				PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
			png_write_info(png, info);

			// convert from 32 bit RGB to 24 bit RGB while saving
			png_byte* src = (png_byte*)bits;
			int srcRowBytes = width * 4;
			int dstRowBytes = width * 3;
			int srcRowOffset = bytesPerRow - srcRowBytes;
			png_byte tempRow[dstRowBytes];
			for (int row = 0; row < height; row++) {
				for (int i = 0; i < dstRowBytes; i += 3, src += 4) {
					tempRow[i]     = src[0];
					tempRow[i + 1] = src[1];
					tempRow[i + 2] = src[2];
				}
				src += srcRowOffset;
				png_write_row(png, tempRow);
			}
			break;
		}
		
		default:
		{
			TRACE(("Unsupported color space\n"));
			png_destroy_write_struct(&png, NULL);
			fclose(file);
			return B_ERROR;
		}
	}

	png_write_end(png, info);
	png_destroy_write_struct(&png, NULL);

	fclose(file);

	// Set the file type manually, so that it doesn't have to be
	// picked up by the registrar or Tracker, first
	BNode node(filename);
	BNodeInfo nodeInfo(&node);
	if (nodeInfo.InitCheck() == B_OK)
		nodeInfo.SetType("image/png");

	return B_OK;
}
