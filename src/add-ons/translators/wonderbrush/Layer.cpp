/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "Layer.h"

#include <stdio.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <Message.h>

#include "bitmap_compression.h"
#include "blending.h"
#include "lab_convert.h"
#include "support.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Layer"


// constructor
Layer::Layer()
	: fBitmap(NULL),
	  fBounds(0.0, 0.0, -1.0, -1.0),
	  fAlpha(1.0),
	  fMode(MODE_NORMAL),
	  fFlags(0)
{
}


// destructor
Layer::~Layer()
{
	delete fBitmap;
}


// Compose
status_t
Layer::Compose(const BBitmap* into, BRect area)
{
	if (!fBitmap || !fBitmap->IsValid()
		|| (fBitmap->ColorSpace() != B_RGBA32
			&& fBitmap->ColorSpace() != B_RGB32))
		return B_NO_INIT;

	status_t status = B_BAD_VALUE;
	if (!into || !area.IsValid() || (status = into->InitCheck()) < B_OK)
		return status;

	// make sure we don't access memory outside of our bitmap
	area = area & fBitmap->Bounds();

	BRect r = ActiveBounds();
	if (!r.IsValid() || (fFlags & FLAG_INVISIBLE) || !r.Intersects(area))
		return B_OK;

	r = r & area;
	int32 left, top, right, bottom;
	rect_to_int(r, left, top, right, bottom);

	uint8* src = (uint8*)fBitmap->Bits();
	uint8* dst = (uint8*)into->Bits();
	uint32 bpr = into->BytesPerRow();
	src += 4 * left + bpr * top;
	dst += 4 * left + bpr * top;
	uint8 alphaOverride = (uint8)(fAlpha * 255);

	switch (fMode) {

		case MODE_SOFT_LIGHT:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						uint8 c1 = dstHandle[0] * srcHandle[0] >> 8;
						c1 += dstHandle[0] * (
								255 - (
									(255 - dstHandle[0])
									* (255 - srcHandle[0])
									>> 8
								) - c1
							) >> 8;
						c1 = (c1 * dstHandle[3]
								+ srcHandle[0] * (255 - dstHandle[3])
							) >> 8;

						uint8 c2 = dstHandle[1] * srcHandle[1] >> 8;
						c2 += dstHandle[1] * (
								255 - (
									(255 - dstHandle[1])
									* (255 - srcHandle[1])
									>> 8
								) - c2
							) >> 8;
						c2 = (c2 * dstHandle[3]
								+ srcHandle[1] * (255 - dstHandle[3])
							) >> 8;

						uint8 c3 = dstHandle[2] * srcHandle[2] >> 8;
						c3 += dstHandle[2] * (
								255 - (
									(255 - dstHandle[2])
									* (255 - srcHandle[2])
									>> 8
								) - c3
							) >> 8;
						c3 = (c3 * dstHandle[3]
								+ srcHandle[2] * (255 - dstHandle[3])
							) >> 8;

						blend_colors(dstHandle,
							(srcHandle[3] * alphaOverride) >> 8, c1, c2, c3);
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_LIGHTEN:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint8 c1
							= (max_c(srcHandle[0], dstHandle[0]) * dstHandle[3]
								+ srcHandle[0] * (255 - dstHandle[3])) / 255;
						uint8 c2
							= (max_c(srcHandle[1], dstHandle[1]) * dstHandle[3]
								+ srcHandle[1] * (255 - dstHandle[3])) / 255;
						uint8 c3
							= (max_c(srcHandle[2], dstHandle[2]) * dstHandle[3]
								+ srcHandle[2] * (255 - dstHandle[3])) / 255;
						blend_colors(dstHandle,
							(srcHandle[3] * alphaOverride) / 255, c1, c2, c3);
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_DARKEN:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint8 c1
							= (min_c(srcHandle[0], dstHandle[0]) * dstHandle[3]
								+ srcHandle[0] * (255 - dstHandle[3])) / 255;
						uint8 c2
							= (min_c(srcHandle[1], dstHandle[1]) * dstHandle[3]
								+ srcHandle[1] * (255 - dstHandle[3])) / 255;
						uint8 c3
							= (min_c(srcHandle[2], dstHandle[2]) * dstHandle[3]
								+ srcHandle[2] * (255 - dstHandle[3])) / 255;
						blend_colors(dstHandle,
							(srcHandle[3] * alphaOverride) / 255, c1, c2, c3);
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_REPLACE_RED:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint32 alpha = srcHandle[3] * alphaOverride;
						dstHandle[2] = (srcHandle[2] * alpha
								+ dstHandle[2] * (65025 - alpha)) / 65025;
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_REPLACE_GREEN:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint32 alpha = srcHandle[3] * alphaOverride;
						dstHandle[1] = (srcHandle[1] * alpha
								+ dstHandle[1] * (65025 - alpha)) / 65025;
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_REPLACE_BLUE:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint32 alpha = srcHandle[3] * alphaOverride;
						dstHandle[0] = (srcHandle[0] * alpha
								+ dstHandle[0] * (65025 - alpha)) / 65025;
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_MULTIPLY_INVERSE_ALPHA:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					// compose
					uint8 temp = min_c(dstHandle[3], 255 - srcHandle[3]);
					dstHandle[3] = (
							dstHandle[3] * (255 - alphaOverride)
							+ temp * alphaOverride
						) / 255;
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_MULTIPLY_ALPHA:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					// compose
					uint8 temp = min_c(dstHandle[3], srcHandle[3]);
					dstHandle[3] = (
							dstHandle[3] * (255 - alphaOverride)
							+ temp * alphaOverride
						) / 255;
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_LUMINANCE:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint8 r = dstHandle[2];
						uint8 g = dstHandle[1];
						uint8 b = dstHandle[0];
						uint8 alpha = dstHandle[3];
						replace_luminance(r, g, b, srcHandle[2], srcHandle[1],
							srcHandle[0]);
						blend_colors(dstHandle,
							(srcHandle[3] * alphaOverride) / 255, b, g, r);
						dstHandle[3] = alpha;
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_INVERSE_MULTIPLY:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint8 c1 = 255 - (
								(((255 - srcHandle[0]) * (255 - dstHandle[0]))
									/ 255) * dstHandle[3]
								+ (255 - srcHandle[0]) * (255 - dstHandle[3])
							) / 255;
						uint8 c2 = 255 - (
								(((255 - srcHandle[1]) * (255 - dstHandle[1]))
									/ 255) * dstHandle[3]
								+ (255 - srcHandle[1]) * (255 - dstHandle[3])
							) / 255;
						uint8 c3 = 255 - (
								(((255 - srcHandle[2]) * (255 - dstHandle[2]))
									/ 255) * dstHandle[3]
								+ (255 - srcHandle[2]) * (255 - dstHandle[3])
							) / 255;
						blend_colors(dstHandle,
							(srcHandle[3] * alphaOverride) / 255, c1, c2, c3);
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_MULTIPLY:
			for (; top <= bottom; top++) {
				uint8* srcHandle = src;
				uint8* dstHandle = dst;
				for (int32 x = left; x <= right; x++) {
					if (srcHandle[3] > 0) {
						// compose
						uint8 c1 = (
								((srcHandle[0] * dstHandle[0]) / 255)
									* dstHandle[3]
								+ srcHandle[0] * (255 - dstHandle[3])
							) / 255;
						uint8 c2 = (
								((srcHandle[1] * dstHandle[1]) / 255)
									* dstHandle[3]
								+ srcHandle[1] * (255 - dstHandle[3])
							) / 255;
						uint8 c3 = (
								((srcHandle[2] * dstHandle[2]) / 255)
									* dstHandle[3]
								+ srcHandle[2] * (255 - dstHandle[3])
							) / 255;
						blend_colors(dstHandle,
							(srcHandle[3] * alphaOverride) / 255, c1, c2, c3);
					}
					srcHandle += 4;
					dstHandle += 4;
				}
				src += bpr;
				dst += bpr;
			}
			break;

		case MODE_NORMAL:
		default:
			if (alphaOverride == 255) {
				// use an optimized version that composes the bitmaps directly
				for (; top <= bottom; top++) {
					uint8* srcHandle = src;
					uint8* dstHandle = dst;
					for (int32 x = left; x <= right; x++) {
						blend_colors(dstHandle, srcHandle);
						srcHandle += 4;
						dstHandle += 4;
					}
					src += bpr;
					dst += bpr;
				}
			} else {
				for (; top <= bottom; top++) {
					uint8* srcHandle = src;
					uint8* dstHandle = dst;
					for (int32 x = left; x <= right; x++) {
						blend_colors(dstHandle, srcHandle, alphaOverride);
						srcHandle += 4;
						dstHandle += 4;
					}
					src += bpr;
					dst += bpr;
				}
			}
			break;
	}

	return status;
}


// Unarchive
status_t
Layer::Unarchive(const BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	// restore attributes
	float alpha;
	if (archive->FindFloat("alpha", &alpha) == B_OK) {
		constrain(alpha, 0.0, 1.0);
		fAlpha = alpha;
	} else
		fAlpha = 1.0;
	if (archive->FindInt32("mode", (int32*)&fMode) < B_OK)
		fMode = MODE_NORMAL;
	if (archive->FindInt32("flags", (int32*)&fFlags) < B_OK)
		fFlags = 0;

	// delete current contents
	delete fBitmap;
	fBitmap = NULL;

	status_t status = extract_bitmap(&fBitmap, archive, "current bitmap");
	if (status < B_OK)
		return status;

	// "bounds" is where the layer actually has content
	BRect bounds;
	if (archive->FindRect("bounds", &bounds) == B_OK)
		fBounds = bounds;
	else
		fBounds.Set(0.0, 0.0, -1.0, -1.0);

	// validate status of fBitmap
	if (!fBitmap)
		return B_ERROR;

	status = fBitmap->InitCheck();
	if (status < B_OK) {
		delete fBitmap;
		fBitmap = NULL;
	}

	return status;
}
