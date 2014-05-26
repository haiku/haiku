/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "AlphaMask.h"

#include "BitmapHWInterface.h"
#include "BitmapManager.h"
#include "DrawingContext.h"
#include "DrawingEngine.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"


AlphaMask::AlphaMask(ServerPicture* picture, bool inverse, BPoint origin,
		const DrawState& drawState)
	:
	fPreviousMask(NULL),

	fPicture(picture),
	fInverse(inverse),
	fOrigin(origin),
	fDrawState(drawState),

	fViewBounds(),
	fViewOffset(),

	fCachedBitmap(NULL),
	fCachedBounds(),
	fCachedOffset(),

	fBuffer(),
	fCachedMask(),
	fScanline(fCachedMask)
{
	fPicture->AcquireReference();
}


AlphaMask::~AlphaMask()
{
	fPicture->ReleaseReference();
	delete[] fCachedBitmap;
	SetPrevious(NULL);
}


void
AlphaMask::Update(BRect bounds, BPoint offset)
{
	fViewBounds = bounds;
	fViewOffset = offset;
	
	if (fPreviousMask != NULL)
		fPreviousMask->Update(bounds, offset);
}


void
AlphaMask::SetPrevious(AlphaMask* mask)
{
	// Since multiple DrawStates can point to the same AlphaMask,
	// don't accept ourself as the "previous" mask on the state stack.
	if (mask == this || mask == fPreviousMask)
		return;

	if (mask != NULL)
		mask->AcquireReference();
	if (fPreviousMask != NULL)
		fPreviousMask->ReleaseReference();
	fPreviousMask = mask;
}


scanline_unpacked_masked_type*
AlphaMask::Generate()
{
	if (fPicture == NULL || !fViewBounds.IsValid())
		return NULL;

	// See if a cached bitmap can be used. Don't use it when the view offset
	// or bounds have changed.
	if (fCachedBitmap != NULL
		&& fViewBounds == fCachedBounds && fViewOffset == fCachedOffset) {
		return &fScanline;
	}

	uint32 width = fViewBounds.IntegerWidth() + 1;
	uint32 height = fViewBounds.IntegerHeight() + 1;

	if (fViewBounds != fCachedBounds || fCachedBitmap == NULL) {
		delete[] fCachedBitmap;
		fCachedBitmap = new(std::nothrow) uint8[width * height];
	}
	
	// If rendering the picture fails, we will draw without any clipping.
	ServerBitmap* bitmap = _RenderPicture();
	if (bitmap == NULL || fCachedBitmap == NULL) {
		fBuffer.attach(NULL, 0, 0, 0);
		return NULL;
	}

	uint8* bits = bitmap->Bits();
	uint32 bytesPerRow = bitmap->BytesPerRow();
	uint8* row = bits;
	uint8* pixel = fCachedBitmap;

	// Let any previous masks also regenerate themselves. Updating the cached
	// mask bitmap is only necessary after the view size changed or the
	// scrolling offset, which definitely affects any masks of lower states
	// as well, so it works recursively until the bottom mask is regenerated.
	bool transferBitmap = true;
	if (fPreviousMask != NULL) {
		fPreviousMask->Generate();
		if (fPreviousMask->fCachedBitmap != NULL) {
			uint8* previousBits = fPreviousMask->fCachedBitmap;
			for (uint32 y = 0; y < height; y++) {
				for (uint32 x = 0; x < width; x++) {
					if (previousBits[0] != 0) {
						if (fInverse)
							pixel[0] = 255 - row[3];
						else
							pixel[0] = row[3];
						pixel[0] = pixel[0] * previousBits[0] / 255;
					} else
						pixel[0] = 0;
					previousBits++;
					pixel++;
					row += 4;
				}
				bits += bytesPerRow;
				row = bits;
			}
			transferBitmap = false;
		}
	}
	
	if (transferBitmap) {
		for (uint32 y = 0; y < height; y++) {
			for (uint32 x = 0; x < width; x++) {
				if (fInverse)
					pixel[0] = 255 - row[3];
				else
					pixel[0] = row[3];
				pixel++;
				row += 4;
			}
			bits += bytesPerRow;
			row = bits;
		}
	}

	bitmap->ReleaseReference();

	fCachedBounds = fViewBounds;
	fCachedOffset = fViewOffset;

	fBuffer.attach(fCachedBitmap, width, height, width);

	fCachedMask.attach(fBuffer, fViewOffset.x + fOrigin.x,
		fViewOffset.y + fOrigin.y, fInverse ? 255 : 0);

	return &fScanline;
}


ServerBitmap*
AlphaMask::_RenderPicture() const
{
	UtilityBitmap* bitmap = new(std::nothrow) UtilityBitmap(fViewBounds,
		B_RGBA32, 0);
	if (bitmap == NULL)
		return NULL;

	if (!bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	// Clear the bitmap with the transparent color
	memset(bitmap->Bits(), 0, bitmap->BitsLength());

	// Render the picture to the bitmap
	BitmapHWInterface interface(bitmap);
	DrawingEngine* engine = interface.CreateDrawingEngine();
	if (engine == NULL) {
		delete bitmap;
		return NULL;
	}

	OffscreenContext context(engine, fDrawState);
	context.PushState();

	if (engine->LockParallelAccess()) {
		// FIXME ConstrainClippingRegion docs says passing NULL disables
		// all clipping. This doesn't work and will crash in Painter.
		BRegion clipping;
		clipping.Include(fViewBounds);
		engine->ConstrainClippingRegion(&clipping);
		fPicture->Play(&context);
		engine->UnlockParallelAccess();
	}

	context.PopState();
	delete engine;

	return bitmap;
}

