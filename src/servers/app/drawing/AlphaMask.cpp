/*
 * Copyright 2014-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */


#include "AlphaMask.h"

#include "BitmapHWInterface.h"
#include "BitmapManager.h"
#include "Canvas.h"
#include "DrawingEngine.h"
#include "PictureBoundingBoxPlayer.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "Shape.h"


// #pragma mark - AlphaMask


AlphaMask::AlphaMask(AlphaMask* previousMask, bool inverse)
	:
	fPreviousMask(previousMask),
	fBounds(),
	fViewOrigin(),
	fInverse(inverse),
	fBackgroundOpacity(0),
	fBits(NULL),
	fBuffer(),
	fMask(),
	fScanline(fMask)
{
}


AlphaMask::AlphaMask(uint8 backgroundOpacity)
	:
	fPreviousMask(),
	fBounds(),
	fViewOrigin(),
	fInverse(false),
	fBackgroundOpacity(backgroundOpacity),
	fBits(NULL),
	fBuffer(),
	fMask(),
	fScanline(fMask)
{
}


AlphaMask::~AlphaMask()
{
	delete[] fBits;
}


IntPoint
AlphaMask::SetViewOrigin(IntPoint viewOrigin)
{
	if (viewOrigin == fViewOrigin)
		return fViewOrigin;

	IntPoint oldOrigin = fViewOrigin;
	fViewOrigin = viewOrigin;

	_AttachMaskToBuffer();

	if (fPreviousMask != NULL)
		fPreviousMask->SetViewOrigin(viewOrigin);

	return oldOrigin;
}


ServerBitmap*
AlphaMask::_CreateTemporaryBitmap(BRect bounds) const
{
	UtilityBitmap* bitmap = new(std::nothrow) UtilityBitmap(bounds,
		B_RGBA32, 0);
	if (bitmap == NULL)
		return NULL;

	if (!bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	memset(bitmap->Bits(), fBackgroundOpacity, bitmap->BitsLength());

	return bitmap;
}


void
AlphaMask::_Generate()
{
	ServerBitmap* const bitmap = _RenderSource();
	BReference<ServerBitmap> bitmapRef(bitmap, true);
	if (bitmap == NULL) {
		_SetNoClipping();
		return;
	}

	const int32 width = fBounds.IntegerWidth() + 1;
	const int32 height = fBounds.IntegerHeight() + 1;

	delete[] fBits;
	fBits = new(std::nothrow) uint8[width * height];

	uint8* source = bitmap->Bits();
	uint8* destination = fBits;
	uint32 numPixels = width * height;

	if (fPreviousMask != NULL) {
		int32 previousStartX = fBounds.left - fPreviousMask->fBounds.left;
		int32 previousStartY = fBounds.top - fPreviousMask->fBounds.top;
		if (previousStartX < 0)
			previousStartX = 0;
		if (previousStartY < 0)
			previousStartY = 0;

		for (int32 y = previousStartY; y < previousStartY + height; y++) {
			uint8* previousRow = fPreviousMask->fBuffer.row_ptr(y);
			for (int32 x = previousStartX; x < previousStartX + width; x++) {
				uint8 sourceAlpha = fInverse ? 255 - source[3] : source[3];
				*destination = sourceAlpha * previousRow[x] / 255;
				destination++;
				source += 4;
			}
		}
	} else {
		while (numPixels--) {
			*destination = fInverse ? 255 - source[3] : source[3];
			destination++;
			source += 4;
		}
	}

	fBuffer.attach(fBits, width, height, width);
	_AttachMaskToBuffer();
}


void
AlphaMask::_SetNoClipping()
{
	fBuffer.attach(NULL, 0, 0, 0);
	_AttachMaskToBuffer();
}


void
AlphaMask::_AttachMaskToBuffer()
{
	uint8 outsideOpacity = fInverse ? 255 - fBackgroundOpacity
		: fBackgroundOpacity;

	AlphaMask* previousMask = fPreviousMask;
	while (previousMask != NULL && outsideOpacity != 0) {
		uint8 previousOutsideOpacity = previousMask->fInverse
			? 255 - previousMask->fBackgroundOpacity
			: previousMask->fBackgroundOpacity;
		outsideOpacity = outsideOpacity * previousOutsideOpacity / 255;
		previousMask = previousMask->fPreviousMask;
	}

	const IntPoint maskOffset = _Offset();
	const int32 offsetX = fBounds.left + maskOffset.x + fViewOrigin.x;
	const int32 offsetY = fBounds.top + maskOffset.y + fViewOrigin.y;

	fMask.attach(fBuffer, offsetX, offsetY, outsideOpacity);
}


// #pragma mark - UniformAlphaMask


UniformAlphaMask::UniformAlphaMask(uint8 opacity)
	:
	AlphaMask(opacity)
{
	fBounds.Set(0, 0, 0, 0);
	_SetNoClipping();
}


ServerBitmap*
UniformAlphaMask::_RenderSource()
{
	return NULL;
}


IntPoint
UniformAlphaMask::_Offset()
{
	return IntPoint(0, 0);
}


// #pragma mark - VectorAlphaMask


template<class VectorMaskType>
VectorAlphaMask<VectorMaskType>::VectorAlphaMask(AlphaMask* previousMask,
	BPoint where, bool inverse)
	:
	AlphaMask(previousMask, inverse),
	fWhere(where)
{
}


template<class VectorMaskType>
ServerBitmap*
VectorAlphaMask<VectorMaskType>::_RenderSource()
{
	fBounds = static_cast<VectorMaskType*>(this)->DetermineBoundingBox();
	if (fPreviousMask != NULL)
		fBounds = fBounds & fPreviousMask->fBounds;
	if (!fBounds.IsValid())
		return NULL;

	ServerBitmap* bitmap = _CreateTemporaryBitmap(fBounds);
	if (bitmap == NULL)
		return NULL;

	// Render the picture to the bitmap
	BitmapHWInterface interface(bitmap);
	DrawingEngine* engine = interface.CreateDrawingEngine();
	if (engine == NULL) {
		bitmap->ReleaseReference();
		return NULL;
	}
	engine->SetRendererOffset(fBounds.left, fBounds.top);

	OffscreenCanvas canvas(engine,
		static_cast<VectorMaskType*>(this)->GetDrawState());

	DrawState* const drawState = canvas.CurrentState();
	drawState->SetDrawingMode(B_OP_ALPHA);
	drawState->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	drawState->SetDrawingModeLocked(true);
	canvas.PushState();

	if (engine->LockParallelAccess()) {
		BRegion clipping;
		clipping.Set((clipping_rect)fBounds);
		engine->ConstrainClippingRegion(&clipping);
		static_cast<VectorMaskType*>(this)->DrawVectors(&canvas);
		engine->UnlockParallelAccess();
	}

	delete engine;

	return bitmap;
}


template<class VectorMaskType>
IntPoint
VectorAlphaMask<VectorMaskType>::_Offset()
{
	return fWhere;
}



// #pragma mark - PictureAlphaMask


PictureAlphaMask::PictureAlphaMask(AlphaMask* previousMask,
	ServerPicture* picture, const DrawState& drawState, BPoint where,
	bool inverse)
	:
	VectorAlphaMask<PictureAlphaMask>(previousMask, where, inverse),
	fPicture(picture),
	fDrawState(new DrawState(drawState))
{
	_Generate();
}


PictureAlphaMask::~PictureAlphaMask()
{
	delete fDrawState;
}


void
PictureAlphaMask::DrawVectors(Canvas* canvas)
{
	fPicture->Play(canvas);
}


BRect
PictureAlphaMask::DetermineBoundingBox() const
{
	BRect boundingBox;
	PictureBoundingBoxPlayer::Play(fPicture, fDrawState, &boundingBox);

	if (!boundingBox.IsValid())
		return boundingBox;

	// Round up and add an additional 2 pixels on the bottom/right to
	// compensate for the various types of rounding used in Painter.
	boundingBox.left = floorf(boundingBox.left);
	boundingBox.right = ceilf(boundingBox.right) + 2;
	boundingBox.top = floorf(boundingBox.top);
	boundingBox.bottom = ceilf(boundingBox.bottom) + 2;

	return boundingBox;
}


const DrawState&
PictureAlphaMask::GetDrawState() const
{
	return *fDrawState;
}


// #pragma mark - ShapeAlphaMask


ShapeAlphaMask::ShapeAlphaMask(AlphaMask* previousMask,  BPoint where,
	bool inverse)
	:
	VectorAlphaMask<ShapeAlphaMask>(previousMask, where, inverse),
	fDrawState()
{
	_Generate();
}


void
ShapeAlphaMask::DrawVectors(Canvas* canvas)
{
	// TODO
}


BRect
ShapeAlphaMask::DetermineBoundingBox() const
{
	// TODO
	return BRect(0, 0, 0, 0);
}


const DrawState&
ShapeAlphaMask::GetDrawState() const
{
	return fDrawState;
}
