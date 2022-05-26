/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "graphics.h"


RasBuf32 gFramebuf = {NULL, 0, 0, 0};


void
Clear(RasBuf32 vb, uint32_t c)
{
	vb.stride -= vb.width;
	for (; vb.height > 0; vb.height--) {
		for (int x = 0; x < vb.width; x++) {
			*vb.colors = c;
			vb.colors++;
		}
		vb.colors += vb.stride;
	}
}


template <typename Color>
RasBuf<Color>
RasBuf<Color>::Clip(int x, int y, int w, int h) const
{
	RasBuf<Color> vb = *this;
	if (x < 0) {w += x; x = 0;}
	if (y < 0) {h += y; y = 0;}
	if (x + w > vb.width) {w = vb.width - x;}
	if (y + h > vb.height) {h = vb.height - y;}
	if (w > 0 && h > 0) {
		vb.colors += y*vb.stride + x;
		vb.width = w;
		vb.height = h;
	} else {
		vb.colors = 0;
		vb.width = 0;
		vb.height = 0;
	}
	return vb;
}

template class RasBuf<uint8>;
template class RasBuf<uint32>;


RasBuf8
Font::ThisGlyph(uint32 ch)
{
	if (ch < firstChar || ch >= firstChar + charCnt)
		return ThisGlyph(' ');

	RasBuf8 rb;
	rb.colors = data + (ch - firstChar) * charWidth * charHeight;
	rb.stride = charWidth;
	rb.width = charWidth;
	rb.height = charHeight;
	return rb;
}


void
BlitMaskRgb(RasBuf32 dst, RasBuf8 src, int32 x, int32 y, uint32_t c)
{
	int dstW, dstH;
	uint32_t dc, a;
	dstW = dst.width; dstH = dst.height;
	dst = dst.Clip(x, y, src.width, src.height);
	src = src.Clip(-x, -y, dstW, dstH);
	dst.stride -= dst.width;
	src.stride -= src.width;
	for (; dst.height > 0; dst.height--) {
		for (x = dst.width; x > 0; x--) {
			dc = *dst.colors;
			a = *src.colors;
			if (a != 0) {
				*dst.colors =
					  ((((dc >>  0) % 0x100) * (0xff - a) / 0xff
						+ ((c >>  0) % 0x100) * a / 0xff) <<  0)
					+ ((((dc >>  8) % 0x100) * (0xff - a) / 0xff
						+ ((c >>  8) % 0x100) * a / 0xff) <<  8)
					+ ((((dc >> 16) % 0x100) * (0xff - a) / 0xff
						+ ((c >> 16) % 0x100) * a / 0xff) << 16)
					+ (dc & 0xff000000);
			}
			dst.colors++;
			src.colors++;
		}
		dst.colors += dst.stride;
		src.colors += src.stride;
	}
}
