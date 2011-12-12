/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "WonderBrushImage.h"

#include "Canvas.h"

WonderBrushImage::WonderBrushImage()
	: fArchive(0UL)
{
}


WonderBrushImage::~WonderBrushImage()
{
}


status_t
WonderBrushImage::SetTo(BPositionIO* stream)
{
	if (!stream)
		return B_BAD_VALUE;

	// try to load the stream as a BMessage and probe it
	// to see whether it might be a WonderBrush image
	fArchive.MakeEmpty();
	status_t status = fArchive.Unflatten(stream);
	if (status < B_OK)
		return status;

	if (fArchive.HasMessage("layer") && fArchive.HasRect("bounds"))
		return B_OK;

	return B_ERROR;
}


BBitmap*
WonderBrushImage::Bitmap() const
{
	Canvas canvas(BRect(0.0, 0.0, -1.0, -1.0));

	if (canvas.Unarchive(&fArchive) < B_OK)
		return NULL;

	return canvas.Bitmap();
}


