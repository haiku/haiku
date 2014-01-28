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
	if (fCachedBitmap)
		fCachedBitmap->ReleaseReference();
}


void
AlphaMask::Update(BRect bounds, BPoint offset)
{
	fViewBounds = bounds;
	fViewOffset = offset;
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

	if (fCachedBitmap != NULL)
		fCachedBitmap->ReleaseReference();
	
	// If rendering the picture fails, we will draw without any clipping.
	ServerBitmap* bitmap = _RenderPicture();
	if (bitmap == NULL) {
		fCachedBitmap = NULL;
		fBuffer.attach(NULL, 0, 0, 0);
		return NULL;
	}

	fCachedBitmap = bitmap;
	fCachedBounds = fViewBounds;
	fCachedOffset = fViewOffset;

	fBuffer.attach(fCachedBitmap->Bits(), fCachedBitmap->Width(),
		fCachedBitmap->Height(), fCachedBitmap->BytesPerRow());

	fCachedMask.attach(fBuffer, fViewOffset.x + fOrigin.x,
		fViewOffset.y + fOrigin.y, fInverse ? 255 : 0);

	return &fScanline;
}


ServerBitmap*
AlphaMask::_RenderPicture() const
{
	// TODO: Only the alpha channel is relevant, but there is no B_ALPHA8
	// color space, so we use 300% more memory than needed.
	UtilityBitmap* bitmap = new(std::nothrow) UtilityBitmap(fViewBounds,
		B_RGBA32, 0);
	if (bitmap == NULL)
		return NULL;

	// Clear the bitmap with the transparent color
	memset(bitmap->Bits(), 0, bitmap->BitsLength());

	// Render the picture to the bitmap
	BitmapHWInterface interface(bitmap);
	DrawingEngine* engine = interface.CreateDrawingEngine();
	if (engine == NULL) {
		delete bitmap;
		return NULL;
	}

	engine->SetDrawState(&fDrawState);

	OffscreenContext context(engine);
	if (engine->LockParallelAccess()) {
		// FIXME ConstrainClippingRegion docs says passing NULL disables
		// all clipping. This doesn't work and will crash in Painter.
		BRegion clipping;
		clipping.Include(fViewBounds);
		engine->ConstrainClippingRegion(&clipping);
		fPicture->Play(&context);
		engine->UnlockParallelAccess();
	}
	delete engine;

	if (!fInverse)
		return bitmap;

	// Compute the inverse of our bitmap. There probably is a better way.
	uint32 size = bitmap->BitsLength();
	uint8* bits = (uint8*)bitmap->Bits();

	for (uint32 i = 3; i < size; i += 4)
		bits[i] = 255 - bits[i];

	return bitmap;
}

