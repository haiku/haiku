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

#include "AlphaMaskCache.h"
#include "BitmapHWInterface.h"
#include "BitmapManager.h"
#include "Canvas.h"
#include "DrawingEngine.h"
#include "PictureBoundingBoxPlayer.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"
#include "Shape.h"
#include "ShapePrivate.h"

#include <AutoLocker.h>


// #pragma mark - AlphaMask


AlphaMask::AlphaMask(AlphaMask* previousMask, bool inverse)
	:
	fPreviousMask(previousMask),
	fBounds(),
	fClippedToCanvas(true),
	fCanvasOrigin(),
	fCanvasBounds(),
	fInverse(inverse),
	fBackgroundOpacity(0),
	fNextMaskCount(0),
	fInCache(false),
	fIndirectCacheReferences(0),
	fBits(NULL),
	fBuffer(),
	fMask(),
	fScanline(fMask)
{
	recursive_lock_init(&fLock, "AlphaMask");

	if (previousMask != NULL)
		atomic_add(&previousMask->fNextMaskCount, 1);

	_SetOutsideOpacity();
}


AlphaMask::AlphaMask(AlphaMask* previousMask, AlphaMask* other)
	:
	fPreviousMask(previousMask),
	fBounds(other->fBounds),
	fClippedToCanvas(other->fClippedToCanvas),
	fCanvasOrigin(other->fCanvasOrigin),
	fCanvasBounds(other->fCanvasBounds),
	fInverse(other->fInverse),
	fBackgroundOpacity(other->fBackgroundOpacity),
	fNextMaskCount(0),
	fInCache(false),
	fIndirectCacheReferences(0),
	fBits(other->fBits),
	fBuffer(other->fBuffer),
	fMask(other->fMask),
	fScanline(fMask)
{
	recursive_lock_init(&fLock, "AlphaMask");

	fMask.attach(fBuffer);

	if (previousMask != NULL)
		atomic_add(&previousMask->fNextMaskCount, 1);

	_SetOutsideOpacity();
}


AlphaMask::AlphaMask(uint8 backgroundOpacity)
	:
	fPreviousMask(),
	fBounds(),
	fClippedToCanvas(true),
	fCanvasOrigin(),
	fCanvasBounds(),
	fInverse(false),
	fBackgroundOpacity(backgroundOpacity),
	fNextMaskCount(0),
	fInCache(false),
	fIndirectCacheReferences(0),
	fBits(NULL),
	fBuffer(),
	fMask(),
	fScanline(fMask)
{
	recursive_lock_init(&fLock, "AlphaMask");

	_SetOutsideOpacity();
}


AlphaMask::~AlphaMask()
{
	if (fPreviousMask.IsSet())
		atomic_add(&fPreviousMask->fNextMaskCount, -1);

	recursive_lock_destroy(&fLock);
}


IntPoint
AlphaMask::SetCanvasGeometry(IntPoint origin, IntRect bounds)
{
	RecursiveLocker locker(fLock);

	if (origin == fCanvasOrigin && bounds.Width() == fCanvasBounds.Width()
		&& bounds.Height() == fCanvasBounds.Height())
		return fCanvasOrigin;

	IntPoint oldOrigin = fCanvasOrigin;
	fCanvasOrigin = origin;
	IntRect oldBounds = fCanvasBounds;
	fCanvasBounds = IntRect(0, 0, bounds.Width(), bounds.Height());

	if (fPreviousMask != NULL)
		fPreviousMask->SetCanvasGeometry(origin, bounds);

	if (fClippedToCanvas && (fCanvasBounds.Width() > oldBounds.Width()
		|| fCanvasBounds.Height() > oldBounds.Height())) {
		// The canvas is now larger than before and we previously
		// drew the alpha mask clipped to the (old) bounds of the
		// canvas. So we now have to redraw the alpha mask with the
		// new size.
		_Generate();
	}

	_AttachMaskToBuffer();

	return oldOrigin;
}


size_t
AlphaMask::BitmapSize() const
{
	return fBits->BitsLength();
}


ServerBitmap*
AlphaMask::_CreateTemporaryBitmap(BRect bounds) const
{
	BReference<UtilityBitmap> bitmap(new(std::nothrow) UtilityBitmap(bounds,
		B_RGBA32, 0), true);
	if (bitmap == NULL)
		return NULL;

	if (!bitmap->IsValid())
		return NULL;

	memset(bitmap->Bits(), fBackgroundOpacity, bitmap->BitsLength());

	return bitmap.Detach();
}


void
AlphaMask::_Generate()
{
	RecursiveLocker locker(fLock);
	RecursiveLocker previousLocker;
	if (fPreviousMask != NULL)
		previousLocker.SetTo(fPreviousMask->fLock, false);

	ServerBitmap* const bitmap = _RenderSource(fCanvasBounds);
	BReference<ServerBitmap> bitmapRef(bitmap, true);
	if (bitmap == NULL) {
		_SetNoClipping();
		return;
	}

	fBits.SetTo(new(std::nothrow) UtilityBitmap(fBounds, B_GRAY8, 0), true);
	if (fBits == NULL)
		return;

	const int32 width = fBits->Width();
	const int32 height = fBits->Height();
	uint8* source = bitmap->Bits();
	uint8* destination = fBits->Bits();
	uint32 numPixels = width * height;

	if (fPreviousMask != NULL) {
		uint8 previousOutsideOpacity = fPreviousMask->OutsideOpacity();

		if (fPreviousMask->fBounds.Intersects(fBounds)) {
			IntRect previousBounds(fBounds.OffsetByCopy(
				-fPreviousMask->fBounds.left, -fPreviousMask->fBounds.top));
			if (previousBounds.right > fPreviousMask->fBounds.Width())
				previousBounds.right = fPreviousMask->fBounds.Width();
			if (previousBounds.bottom > fPreviousMask->fBounds.Height())
				previousBounds.bottom = fPreviousMask->fBounds.Height();

			int32 y = previousBounds.top;

			for (; y < 0; y++) {
				for (int32 x = 0; x < width; x++) {
					*destination = (fInverse ? 255 - source[3] : source[3])
						* previousOutsideOpacity / 255;
					destination++;
					source += 4;
				}
			}

			for (; y <= previousBounds.bottom; y++) {
				int32 x = previousBounds.left;
				for (; x < 0; x++) {
					*destination = (fInverse ? 255 - source[3] : source[3])
						* previousOutsideOpacity / 255;
					destination++;
					source += 4;
				}
				uint8* previousRow = fPreviousMask->fBuffer.row_ptr(y);
				for (; x <= previousBounds.right; x++) {
					uint8 sourceAlpha = fInverse ? 255 - source[3] : source[3];
					*destination = sourceAlpha * previousRow[x] / 255;
					destination++;
					source += 4;
				}
				for (; x < previousBounds.left + width; x++) {
					*destination = (fInverse ? 255 - source[3] : source[3])
						* previousOutsideOpacity / 255;
					destination++;
					source += 4;
				}
			}

			for (; y < previousBounds.top + height; y++) {
				for (int32 x = 0; x < width; x++) {
					*destination = (fInverse ? 255 - source[3] : source[3])
						* previousOutsideOpacity / 255;
					destination++;
					source += 4;
				}
			}

		} else {
			while (numPixels--) {
				*destination = (fInverse ? 255 - source[3] : source[3])
					* previousOutsideOpacity / 255;
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

	fBuffer.attach(fBits->Bits(), width, height, width);
	_AttachMaskToBuffer();

	_AddToCache();
}


void
AlphaMask::_SetNoClipping()
{
	fBuffer.attach(NULL, 0, 0, 0);
	_AttachMaskToBuffer();
}


const IntRect&
AlphaMask::_PreviousMaskBounds() const
{
	return fPreviousMask->fBounds;
}


void
AlphaMask::_AttachMaskToBuffer()
{
	const IntPoint maskOffset = _Offset();
	const int32 offsetX = fBounds.left + maskOffset.x + fCanvasOrigin.x;
	const int32 offsetY = fBounds.top + maskOffset.y + fCanvasOrigin.y;

	fMask.attach(fBuffer, offsetX, offsetY, fOutsideOpacity);
}


void
AlphaMask::_SetOutsideOpacity()
{
	fOutsideOpacity = fInverse ? 255 - fBackgroundOpacity
		: fBackgroundOpacity;

	if (fPreviousMask != NULL) {
		fOutsideOpacity = fOutsideOpacity * fPreviousMask->OutsideOpacity()
			/ 255;
	}
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
UniformAlphaMask::_RenderSource(const IntRect&)
{
	return NULL;
}


IntPoint
UniformAlphaMask::_Offset()
{
	return IntPoint(0, 0);
}


void
UniformAlphaMask::_AddToCache()
{
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
VectorAlphaMask<VectorMaskType>::VectorAlphaMask(AlphaMask* previousMask,
	VectorAlphaMask* other)
	:
	AlphaMask(previousMask, other),
	fWhere(other->fWhere)
{
}


template<class VectorMaskType>
ServerBitmap*
VectorAlphaMask<VectorMaskType>::_RenderSource(const IntRect& canvasBounds)
{
	fBounds = static_cast<VectorMaskType*>(this)->DetermineBoundingBox();

	if (fBounds.Width() > canvasBounds.Width()
		|| fBounds.Height() > canvasBounds.Height()) {
		fBounds = fBounds & canvasBounds;
		fClippedToCanvas = true;
	} else
		fClippedToCanvas = false;

	if (fPreviousMask != NULL) {
		if (IsInverted()) {
			if (fPreviousMask->OutsideOpacity() != 0) {
				IntRect previousBounds = _PreviousMaskBounds();
				if (previousBounds.IsValid())
					fBounds = fBounds | previousBounds;
			} else
				fBounds = _PreviousMaskBounds();
			fClippedToCanvas = fClippedToCanvas || fPreviousMask->IsClipped();
		} else if (fPreviousMask->OutsideOpacity() == 0)
			fBounds = fBounds & _PreviousMaskBounds();
	}
	if (!fBounds.IsValid())
		return NULL;

	BReference<ServerBitmap> bitmap(_CreateTemporaryBitmap(fBounds), true);
	if (bitmap == NULL)
		return NULL;

	// Render the picture to the bitmap
	BitmapHWInterface interface(bitmap);
	ObjectDeleter<DrawingEngine> engine(interface.CreateDrawingEngine());
	if (!engine.IsSet())
		return NULL;

	engine->SetRendererOffset(fBounds.left, fBounds.top);

	OffscreenCanvas canvas(engine.Get(),
		static_cast<VectorMaskType*>(this)->GetDrawState(), fBounds);

	DrawState* const drawState = canvas.CurrentState();
	drawState->SetDrawingMode(B_OP_ALPHA);
	drawState->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	drawState->SetDrawingModeLocked(true);
	canvas.PushState();

	canvas.ResyncDrawState();

	if (engine->LockParallelAccess()) {
		BRegion clipping;
		clipping.Set((clipping_rect)fBounds);
		engine->ConstrainClippingRegion(&clipping);
		static_cast<VectorMaskType*>(this)->DrawVectors(&canvas);
		engine->UnlockParallelAccess();
	}

	canvas.PopState();

	return bitmap.Detach();
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
	fDrawState(new(std::nothrow) DrawState(drawState))
{
}


PictureAlphaMask::~PictureAlphaMask()
{
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
	PictureBoundingBoxPlayer::Play(fPicture, fDrawState.Get(), &boundingBox);

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
	return *fDrawState.Get();
}


void
PictureAlphaMask::_AddToCache()
{
	// currently not implemented
}


// #pragma mark - ShapeAlphaMask


DrawState* ShapeAlphaMask::fDrawState = NULL;


ShapeAlphaMask::ShapeAlphaMask(AlphaMask* previousMask,
	const shape_data& shape, BPoint where, bool inverse)
	:
	VectorAlphaMask<ShapeAlphaMask>(previousMask, where, inverse),
	fShape(new(std::nothrow) shape_data(shape), true)
{
	if (fDrawState == NULL)
		fDrawState = new(std::nothrow) DrawState();

	fShapeBounds = fShape->DetermineBoundingBox();
}


ShapeAlphaMask::ShapeAlphaMask(AlphaMask* previousMask,
	ShapeAlphaMask* other)
	:
	VectorAlphaMask<ShapeAlphaMask>(previousMask, other),
	fShape(other->fShape),
	fShapeBounds(other->fShapeBounds)
{
}


ShapeAlphaMask::~ShapeAlphaMask()
{
}


/* static */ ShapeAlphaMask*
ShapeAlphaMask::Create(AlphaMask* previousMask, const shape_data& shape,
	BPoint where, bool inverse)
{
	// Look if we have a suitable cached mask
	BReference<ShapeAlphaMask> mask(AlphaMaskCache::Default()->Get(shape,
		previousMask, inverse), true);

	if (mask == NULL) {
		// No cached mask, create new one
		mask.SetTo(new(std::nothrow) ShapeAlphaMask(previousMask, shape,
			BPoint(0, 0), inverse), true);
	} else {
		// Create new mask which reuses the parameters and the mask bitmap
		// of the cache entry
		// TODO: don't make a new mask if the cache entry has no drawstate
		// using it anymore, because then we ca just immediately reuse it
		RecursiveLocker locker(mask->fLock);
		mask.SetTo(new(std::nothrow) ShapeAlphaMask(previousMask, mask), true);
	}

	return mask.Detach();
}


void
ShapeAlphaMask::DrawVectors(Canvas* canvas)
{
	canvas->GetDrawingEngine()->DrawShape(fBounds,
		fShape->opCount, fShape->opList,
		fShape->ptCount, fShape->ptList,
		true, BPoint(0, 0), 1.0);
}


BRect
ShapeAlphaMask::DetermineBoundingBox() const
{
	return fShapeBounds;
}


const DrawState&
ShapeAlphaMask::GetDrawState() const
{
	return *fDrawState;
}


void
ShapeAlphaMask::_AddToCache()
{
	AlphaMaskCache::Default()->Put(this);
}
