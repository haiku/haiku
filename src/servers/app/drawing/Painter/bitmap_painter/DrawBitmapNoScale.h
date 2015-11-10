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
#include "IntRect.h"
#include "Painter.h"
#include "SystemPalette.h"


template<class BlendType>
struct DrawBitmapNoScale {
public:
	void
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

		fColorMap = SystemPalette();
		fAlphaMask = aggInterface.fClippedAlphaMask;
		renderer_base& baseRenderer = aggInterface.fBaseRenderer;

		// copy rects, iterate over clipping boxes
		baseRenderer.first_clip_box();
		do {
			fRect.left  = max_c(baseRenderer.xmin(), left);
			fRect.right = min_c(baseRenderer.xmax(), right);
			if (fRect.left <= fRect.right) {
				fRect.top    = max_c(baseRenderer.ymin(), top);
				fRect.bottom = min_c(baseRenderer.ymax(), bottom);
				if (fRect.top <= fRect.bottom) {
					uint8* dstHandle = dst + fRect.top * dstBPR
						+ fRect.left * 4;
					const uint8* srcHandle = src
						+ (fRect.top  - offset.y) * srcBPR
						+ (fRect.left - offset.x) * bytesPerSourcePixel;

					for (; fRect.top <= fRect.bottom; fRect.top++) {
						static_cast<BlendType*>(this)->BlendRow(dstHandle,
							srcHandle, fRect.right - fRect.left + 1);

						dstHandle += dstBPR;
						srcHandle += srcBPR;
					}
				}
			}
		} while (baseRenderer.next_clip_box());
	}

protected:
	IntRect fRect;
	const rgb_color* fColorMap;
	const agg::clipped_alpha_mask* fAlphaMask;
};


struct CMap8Copy : public DrawBitmapNoScale<CMap8Copy>
{
	void BlendRow(uint8* dst, const uint8* src, int32 numPixels)
	{
		uint32* d = (uint32*)dst;
		const uint8* s = src;
		while (numPixels--) {
			const rgb_color c = fColorMap[*s++];
			*d++ = (c.alpha << 24) | (c.red << 16) | (c.green << 8) | (c.blue);
		}
	}
};


struct CMap8Over : public DrawBitmapNoScale<CMap8Over>
{
	void BlendRow(uint8* dst, const uint8* src, int32 numPixels)
	{
		uint32* d = (uint32*)dst;
		const uint8* s = src;
		while (numPixels--) {
			const rgb_color c = fColorMap[*s++];
			if (c.alpha)
				*d = (c.alpha << 24) | (c.red << 16)
					| (c.green << 8) | (c.blue);
			d++;
		}
	}
};


struct Bgr32Copy : public DrawBitmapNoScale<Bgr32Copy>
{
	void BlendRow(uint8* dst, const uint8* src, int32 numPixels)
	{
		memcpy(dst, src, numPixels * 4);
	}
};


struct Bgr32Over : public DrawBitmapNoScale<Bgr32Over>
{
	void BlendRow(uint8* dst, const uint8* src, int32 numPixels)
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
	void BlendRow(uint8* dst, const uint8* src, int32 numPixels)
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


struct Bgr32CopyMasked : public DrawBitmapNoScale<Bgr32CopyMasked>
{
	void BlendRow(uint8* dst, const uint8* src, int32 numPixels)
	{
		uint8 covers[numPixels];
		fAlphaMask->get_hspan(fRect.left, fRect.top, covers, numPixels);

		uint32* destination = (uint32*)dst;
		uint32* source = (uint32*)src;
		uint8* mask = (uint8*)&covers[0];

		while (numPixels--) {
			if (*mask != 0)
				*destination = *source;
			destination++;
			source++;
			mask++;
		}
	}
};


#endif // DRAW_BITMAP_NO_SCALE_H
