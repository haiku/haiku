/*
 * Copyright 2009, Christian Packmann.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * Copyright 2005-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2015, Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DRAW_BITMAP_NO_SCALE_H
#define DRAW_BITMAP_NO_SCALE_H

#include "IntPoint.h"
#include "Painter.h"


template<class BlendType>
struct DrawBitmapNoScale {
	static void
	Draw(PainterAggInterface& aggInterface, agg::rendering_buffer& bitmap,
		uint32 bytesPerSourcePixel, IntPoint offset, BRect destinationRect)
	{
		// NOTE: this would crash if destinationRect was large enough to read
		// outside the bitmap, so make sure this is not the case before calling
		// this function!
		uint8* dst = aggInterface.fBuffer.row_ptr(0);
		const uint32 dstBPR = aggInterface.fBuffer.stride();

		const uint8* src = bitmap.row_ptr(0);
		const uint32 srcBPR = bitmap.stride();

		const int32 left = (int32)destinationRect.left;
		const int32 top = (int32)destinationRect.top;
		const int32 right = (int32)destinationRect.right;
		const int32 bottom = (int32)destinationRect.bottom;

#if DEBUG_DRAW_BITMAP
	if (left - offset.x < 0
		|| left  - offset.x >= (int32)bitmap.width()
		|| right - offset.x >= (int32)srcBuffer.width()
		|| top - offset.y < 0
		|| top - offset.y >= (int32)bitmap.height()
		|| bottom - offset.y >= (int32)bitmap.height()) {
		char message[256];
		sprintf(message, "reading outside of bitmap (%ld, %ld, %ld, %ld) "
				"(%d, %d) (%ld, %ld)",
			left - offset.x, top - offset.y,
			right - offset.x, bottom - offset.y,
			bitmap.width(), bitmap.height(), offset.x, offset.y);
		debugger(message);
	}
#endif

		const rgb_color* colorMap = SystemPalette();
		renderer_base& baseRenderer = aggInterface.fBaseRenderer;

		// copy rects, iterate over clipping boxes
		baseRenderer.first_clip_box();
		do {
			int32 x1 = max_c(baseRenderer.xmin(), left);
			int32 x2 = min_c(baseRenderer.xmax(), right);
			if (x1 <= x2) {
				int32 y1 = max_c(baseRenderer.ymin(), top);
				int32 y2 = min_c(baseRenderer.ymax(), bottom);
				if (y1 <= y2) {
					uint8* dstHandle = dst + y1 * dstBPR + x1 * 4;
					const uint8* srcHandle = src + (y1 - offset.y) * srcBPR
						+ (x1 - offset.x) * bytesPerSourcePixel;

					for (; y1 <= y2; y1++) {
						BlendType::BlendRow(dstHandle, srcHandle,
							x2 - x1 + 1, colorMap);

						dstHandle += dstBPR;
						srcHandle += srcBPR;
					}
				}
			}
		} while (baseRenderer.next_clip_box());
	}
};


struct CMap8Copy : public DrawBitmapNoScale<CMap8Copy>
{
	static void BlendRow(uint8* dst, const uint8* src, int32 numPixels,
		const rgb_color* colorMap)
	{
		uint32* d = (uint32*)dst;
		const uint8* s = src;
		while (numPixels--) {
			const rgb_color c = colorMap[*s++];
			*d++ = (c.alpha << 24) | (c.red << 16) | (c.green << 8) | (c.blue);
		}
	}
};


struct CMap8Over : public DrawBitmapNoScale<CMap8Over>
{
	static void BlendRow(uint8* dst, const uint8* src, int32 numPixels,
		const rgb_color* colorMap)
	{
		uint32* d = (uint32*)dst;
		const uint8* s = src;
		while (numPixels--) {
			const rgb_color c = colorMap[*s++];
			if (c.alpha)
				*d = (c.alpha << 24) | (c.red << 16)
					| (c.green << 8) | (c.blue);
			d++;
		}
	}
};


struct Bgr32Copy : public DrawBitmapNoScale<Bgr32Copy>
{
	static void BlendRow(uint8* dst, const uint8* src, int32 numPixels,
		const rgb_color*)
	{
		memcpy(dst, src, numPixels * 4);
	}
};


struct Bgr32Over : public DrawBitmapNoScale<Bgr32Over>
{
	static void BlendRow(uint8* dst, const uint8* src, int32 numPixels,
		const rgb_color*)
	{
		uint32* d = (uint32*)dst;
		uint32* s = (uint32*)src;
		while (numPixels--) {
			if (*s != B_TRANSPARENT_MAGIC_RGBA32)
				*(uint32*)d = *(uint32*)s;
			d++;
			s++;
		}
	}
};


struct Bgr32Alpha : public DrawBitmapNoScale<Bgr32Alpha>
{
	static void BlendRow(uint8* dst, const uint8* src, int32 numPixels,
		const rgb_color*)
	{
		uint32* d = (uint32*)dst;
		int32 bytes = numPixels * 4;
		uint8 buffer[bytes];
		uint8* b = buffer;
		while (numPixels--) {
			if (src[3] == 255) {
				*(uint32*)b = *(uint32*)src;
			} else {
				*(uint32*)b = *d;
				b[0] = ((src[0] - b[0]) * src[3] + (b[0] << 8)) >> 8;
				b[1] = ((src[1] - b[1]) * src[3] + (b[1] << 8)) >> 8;
				b[2] = ((src[2] - b[2]) * src[3] + (b[2] << 8)) >> 8;
			}
			d++;
			b += 4;
			src += 4;
		}
		memcpy(dst, buffer, bytes);
	}
};


#endif // DRAW_BITMAP_NO_SCALE_H
