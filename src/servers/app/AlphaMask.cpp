/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */


#include "AlphaMask.h"

#include "BitmapHWInterface.h"
#include "BitmapManager.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "ServerBitmap.h"
#include "View.h"


AlphaMask::AlphaMask(View* view, ServerPicture* picture, bool inverse,
		BPoint origin)
	:
	fPicture(picture),
	fInverse(inverse),
	fOrigin(origin),
	fView(view),
	fCachedBitmap(NULL),
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


scanline_unpacked_masked_type*
AlphaMask::Generate()
{
//	if (fCachedBitmap != NULL) {
//		// TODO: See if cached bitmap can actually be used. Don't use it
//		// when view scrolling offset has changed, for example. Generate()
//		// could be passed a current offset
//		return &fScanline;
//	}

	if (fCachedBitmap != NULL)
		fCachedBitmap->ReleaseReference();
	
	// If rendering the picture fails, we will draw without any clipping.
	ServerBitmap* bitmap = _RenderPicture(fPicture, fInverse);
	if (bitmap == NULL) {
		fCachedBitmap = NULL;
		fBuffer.attach(NULL, 0, 0, 0);
		return NULL;
	}

	fCachedBitmap = bitmap;

	fBuffer.attach(fCachedBitmap->Bits(), fCachedBitmap->Width(),
		fCachedBitmap->Height(), fCachedBitmap->BytesPerRow());

	BPoint offset(B_ORIGIN);
	fView->ConvertToScreen(&offset);

	fCachedMask.attach(fBuffer, offset.x + fOrigin.x, offset.y + fOrigin.y,
		fInverse ? 255 : 0);

	return &fScanline;
}


ServerBitmap*
AlphaMask::_RenderPicture(ServerPicture* picture, bool inverse) const
{
	BRect bounds(fView->Bounds());

	// TODO: Only the alpha channel is relevant, but there is no B_ALPHA8
	// color space, so we use 300% more memory than needed.
	UtilityBitmap* bitmap = new(std::nothrow) UtilityBitmap(bounds, B_RGBA32,
		0);
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

	// Copy the current state of the client view, so we draw with the right
	// font, color and everything
	DrawState drawState(*fView->CurrentState());
	engine->SetDrawState(&drawState);

	OffscreenContext context(engine);
	if (engine->LockParallelAccess()) {
		// FIXME ConstrainClippingRegion docs says passing NULL disables
		// all clipping. This doesn't work and will crash in Painter.
		BRegion clipping;
		clipping.Include(bounds);
		engine->ConstrainClippingRegion(&clipping);
		picture->Play(&context);
		engine->UnlockParallelAccess();
	}
	delete engine;

	if (!inverse)
		return bitmap;

	// Compute the inverse of our bitmap. There probably is a better way.
	uint32 size = bitmap->BitsLength();
	uint8* bits = (uint8*)bitmap->Bits();

	for(uint32 i = 0; i < size; i++)
		bits[i] = 255 - bits[i];

	return bitmap;
}

