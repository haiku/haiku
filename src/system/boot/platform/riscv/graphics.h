/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_


#include <SupportDefs.h>


template <typename Color>
struct RasBuf {
	Color* colors;
	int32 stride, width, height;

	RasBuf<Color> Clip(int x, int y, int w, int h) const;
};

typedef RasBuf<uint8>  RasBuf8;
typedef RasBuf<uint32> RasBuf32;


struct Font {
	uint8 charWidth;
	uint8 charHeight;
	uint8 firstChar, charCnt;
	uint8 data[0];

	RasBuf8 ThisGlyph(uint32 ch);
};


extern RasBuf32 gFramebuf;
extern Font gFixedFont;


void Clear(RasBuf32 vb, uint32_t c);

void BlitMaskRgb(RasBuf32 dst, RasBuf8 src, int32 x, int32 y, uint32_t c);


#endif	// _GRAPHICS_H_
