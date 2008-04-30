/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef PNGDUMP_H
#define PNGDUMP_H


#include <GraphicsDefs.h>

class BRect;


status_t SaveToPNG(const char* filename, const BRect& bounds, color_space space, 
			const void* bits, int32 bitsLength, int32 bytesPerRow);

#endif
