/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "Layer.h"

#include "AlphaMask.h"
#include "BitmapHWInterface.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "IntRect.h"
#include "PictureBoundingBoxPlayer.h"
#include "ServerBitmap.h"
#include "View.h"


class LayerCanvas : public Canvas {
public:
	LayerCanvas(DrawingEngine* drawingEngine, DrawState* drawState,
		BRect bitmapBounds)
		:
		Canvas(),
		fDrawingEngine(drawingEngine),
		fBitmapBounds(bitmapBounds)
	{
		fDrawState.SetTo(drawState);
	}

	virtual DrawingEngine* GetDrawingEngine() const
	{
		return fDrawingEngine;
	}

	virtual ServerPicture* GetPicture(int32 token) const
	{
		return NULL;
	}

	virtual void RebuildClipping(bool)
	{
	}

	virtual void ResyncDrawState()
	{
		fDrawingEngine->SetDrawState(fDrawState.Get());
	}

	virtual void UpdateCurrentDrawingRegion()
	{
		bool hasDrawStateClipping = fDrawState->GetCombinedClippingRegion(
			&fCurrentDrawingRegion);

		BRegion bitmapRegion(fBitmapBounds);
		if (hasDrawStateClipping)
			fCurrentDrawingRegion.IntersectWith(&bitmapRegion);
		else
			fCurrentDrawingRegion = bitmapRegion;

		fDrawingEngine->ConstrainClippingRegion(&fCurrentDrawingRegion);
	}

	virtual	IntRect Bounds() const
	{
		return fBitmapBounds;
	}

protected:
	virtual void _LocalToScreenTransform(SimpleTransform&) const
	{
	}

	virtual void _ScreenToLocalTransform(SimpleTransform&) const
	{
	}

private:
	DrawingEngine*	fDrawingEngine;
	BRegion			fCurrentDrawingRegion;
	BRect			fBitmapBounds;
};


Layer::Layer(uint8 opacity)
	:
	fOpacity(opacity),
	fLeftTopOffset(0, 0)
{
}


Layer::~Layer()
{
}


void
Layer::PushLayer(Layer* layer)
{
	PushPicture(layer);
}


Layer*
Layer::PopLayer()
{
	return static_cast<Layer*>(PopPicture());
}


UtilityBitmap*
Layer::RenderToBitmap(Canvas* canvas)
{
	BRect boundingBox = _DetermineBoundingBox(canvas);
	if (!boundingBox.IsValid())
		return NULL;

	fLeftTopOffset = boundingBox.LeftTop();

	BReference<UtilityBitmap> layerBitmap(_AllocateBitmap(boundingBox), true);
	if (layerBitmap == NULL)
		return NULL;

	BitmapHWInterface layerInterface(layerBitmap);
	ObjectDeleter<DrawingEngine> const layerEngine(layerInterface.CreateDrawingEngine());
	if (layerEngine.Get() == NULL)
		return NULL;

	layerEngine->SetRendererOffset(boundingBox.left, boundingBox.top);
		// Drawing commands of the layer's picture use coordinates in the
		// coordinate space of the underlying canvas. The coordinate origin
		// of the layer bitmap is at boundingBox.LeftTop(). So all the drawing
		// from the picture needs to be offset to be moved into the bitmap.
		// We use a low-level offsetting via the AGG renderer here because the
		// offset needs to be processed independently, after all other
		// transforms, even after the BAffineTransforms (which are processed in
		// Painter), to prevent this origin from being further transformed by
		// e.g. scaling.

	LayerCanvas layerCanvas(layerEngine.Get(), canvas->DetachDrawState(), boundingBox);

	AlphaMask* const mask = layerCanvas.GetAlphaMask();
	IntPoint oldOffset;
	if (mask != NULL) {
		// Move alpha mask to bitmap origin
		oldOffset = mask->SetCanvasGeometry(IntPoint(0, 0), boundingBox);
	}

	layerCanvas.CurrentState()->SetDrawingMode(B_OP_ALPHA);
	layerCanvas.CurrentState()->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	layerCanvas.ResyncDrawState();
		// Apply state to the new drawing engine of the layer canvas

	if (layerEngine->LockParallelAccess()) {
		layerCanvas.UpdateCurrentDrawingRegion();

		// Draw recorded picture into bitmap
		Play(&layerCanvas);
		layerEngine->UnlockParallelAccess();
	}

	if (mask != NULL) {
		// Move alpha mask back to its old position
		// Note: this needs to be adapted if setting alpha masks is
		// implemented as BPicture command (the mask now might be a different
		// one than before).
		layerCanvas.CurrentState()->CombinedTransform().Apply(oldOffset);
		mask->SetCanvasGeometry(oldOffset, boundingBox);
		layerCanvas.ResyncDrawState();
	}

	canvas->SetDrawState(layerCanvas.DetachDrawState());
		// Update state in canvas (the top-of-stack state could be a different
		// state instance now, if the picture commands contained push/pop
		// commands)

	return layerBitmap.Detach();
}


IntPoint
Layer::LeftTopOffset() const
{
	return fLeftTopOffset;
}


uint8
Layer::Opacity() const
{
	return fOpacity;
}


BRect
Layer::_DetermineBoundingBox(Canvas* canvas)
{
	BRect boundingBox;
	PictureBoundingBoxPlayer::Play(this, canvas->CurrentState(), &boundingBox);

	if (!boundingBox.IsValid())
		return boundingBox;

	// Round up and add an additional 2 pixels on the bottom/right to
	// compensate for the various types of rounding used in Painter.
	boundingBox.left = floorf(boundingBox.left);
	boundingBox.right = ceilf(boundingBox.right) + 2;
	boundingBox.top = floorf(boundingBox.top);
	boundingBox.bottom = ceilf(boundingBox.bottom) + 2;

	// TODO: for optimization, crop the bounding box to the underlying
	// view bounds here

	return boundingBox;
}


UtilityBitmap*
Layer::_AllocateBitmap(const BRect& bounds)
{
	BReference<UtilityBitmap> layerBitmap(new(std::nothrow) UtilityBitmap(bounds,
		B_RGBA32, 0), true);
	if (layerBitmap == NULL)
		return NULL;
	if (!layerBitmap->IsValid())
		return NULL;

	memset(layerBitmap->Bits(), 0, layerBitmap->BitsLength());

	return layerBitmap.Detach();
}
