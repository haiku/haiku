/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "AlphaMask.h"

#include "ServerBitmap.h"
#include "View.h"


AlphaMask::AlphaMask(View& view, ServerPicture& picture, bool inverse,
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
	fPicture.AcquireReference();
}


AlphaMask::~AlphaMask()
{
	fPicture.ReleaseReference();
	if (fCachedBitmap)
		fCachedBitmap->ReleaseReference();
}


scanline_unpacked_masked_type*
AlphaMask::Generate()
{
	// If rendering the picture fails, we will draw without any clipping.
	ServerBitmap* bitmap = fView._RenderPicture(&fPicture, fInverse);
	if (!bitmap)
		return NULL;

	// FIXME actually use the cached bitmap whenever possible, instead of
	// rendering the BPicture again and again.
	if (fCachedBitmap)
		fCachedBitmap->ReleaseReference();
	fCachedBitmap = bitmap;

	fBuffer.attach(fCachedBitmap->Bits(), fCachedBitmap->Width(),
		fCachedBitmap->Height(), fCachedBitmap->BytesPerRow());

	BPoint offset(B_ORIGIN);
	fView.ConvertToScreen(&offset);
	fCachedMask.attach(fBuffer, offset.x + fOrigin.x, offset.y + fOrigin.y,
		fInverse ? 255 : 0);
	return &fScanline;
}
