/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	Function for saving a generic framebuffer to a PNG file */

#include "PNGDump.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <png.h>

#include <Rect.h>
#include <OS.h>

#include "frame_buffer_support.h"

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

	// TODO: Don't assume B_BGRA32!
   	png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	png_set_strip_alpha(png);

	png_write_info(png, info);

#if 1
	png_byte* rowPointers[height];
	png_byte* index = (png_byte*)bits;
	for (int32 i = 0; i < height; i++) {
		rowPointers[i] = index;
		index += bytesPerRow;
	}

	png_write_image(png, rowPointers);

#else

	png_byte* index = (png_byte*)bits;
	int32 visibleBytes = width * 4;
	png_byte tempRow[visibleBytes];
	for (int32 i = 0; i < height; i++) {
		gfxcpy32(tempRow, index, visibleBytes);
		png_write_row(png, tempRow);
		index += bytesPerRow;
	}

#endif

	png_write_end(png, info);
	png_destroy_write_struct(&png, NULL);

	fclose(file);
	return B_OK;
}
