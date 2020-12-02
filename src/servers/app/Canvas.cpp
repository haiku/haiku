/*
 * Copyright (c) 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */


#include "Canvas.h"

#include <new>

#include <Region.h>

#include "AlphaMask.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "Layer.h"


#if __GNUC__ >= 3
#	define GCC_2_NRV(x)
	// GCC >= 3.1 doesn't need it anymore
#else
#	define GCC_2_NRV(x) return x;
	// GCC 2 named return value syntax
	// see http://gcc.gnu.org/onlinedocs/gcc-2.95.2/gcc_5.html#SEC106
#endif


Canvas::Canvas()
	:
	fDrawState(new(std::nothrow) DrawState())
{
}


Canvas::Canvas(const DrawState& state)
	:
	fDrawState(new(std::nothrow) DrawState(state))
{
}


Canvas::~Canvas()
{
}


status_t
Canvas::InitCheck() const
{
	if (fDrawState.Get() == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
Canvas::PushState()
{
	DrawState* previous = fDrawState.Detach();
	DrawState* newState = previous->PushState();
	if (newState == NULL)
		newState = previous;

	fDrawState.SetTo(newState);
}


void
Canvas::PopState()
{
	if (fDrawState->PreviousState() == NULL)
		return;

	bool rebuildClipping = fDrawState->HasAdditionalClipping();

	fDrawState.SetTo(fDrawState->PopState());

	// rebuild clipping
	// (the clipping from the popped state is not effective anymore)
	if (rebuildClipping)
		RebuildClipping(false);
}


void
Canvas::SetDrawState(DrawState* newState)
{
	fDrawState.SetTo(newState);
}


void
Canvas::SetDrawingOrigin(BPoint origin)
{
	fDrawState->SetOrigin(origin);

	// rebuild clipping
	if (fDrawState->HasClipping())
		RebuildClipping(false);
}


BPoint
Canvas::DrawingOrigin() const
{
	BPoint origin(fDrawState->Origin());
	float scale = Scale();

	origin.x *= scale;
	origin.y *= scale;

	return origin;
}


void
Canvas::SetScale(float scale)
{
	fDrawState->SetScale(scale);

	// rebuild clipping
	if (fDrawState->HasClipping())
		RebuildClipping(false);
}


float
Canvas::Scale() const
{
	return fDrawState->Scale();
}


void
Canvas::SetUserClipping(const BRegion* region)
{
	fDrawState->SetClippingRegion(region);

	// rebuild clipping (for just this canvas)
	RebuildClipping(false);
}


bool
Canvas::ClipToRect(BRect rect, bool inverse)
{
	bool needDrawStateUpdate = fDrawState->ClipToRect(rect, inverse);
	RebuildClipping(false);
	return needDrawStateUpdate;
}


void
Canvas::ClipToShape(shape_data* shape, bool inverse)
{
	fDrawState->ClipToShape(shape, inverse);
}


void
Canvas::SetAlphaMask(AlphaMask* mask)
{
	fDrawState->SetAlphaMask(mask);
}


AlphaMask*
Canvas::GetAlphaMask() const
{
	return fDrawState->GetAlphaMask();
}


SimpleTransform
Canvas::LocalToScreenTransform() const GCC_2_NRV(transform)
{
#if __GNUC__ >= 3
	SimpleTransform transform;
#endif
	_LocalToScreenTransform(transform);
	return transform;
}


SimpleTransform
Canvas::ScreenToLocalTransform() const GCC_2_NRV(transform)
{
#if __GNUC__ >= 3
	SimpleTransform transform;
#endif
	_ScreenToLocalTransform(transform);
	return transform;
}


SimpleTransform
Canvas::PenToScreenTransform() const GCC_2_NRV(transform)
{
#if __GNUC__ >= 3
	SimpleTransform transform;
#endif
	fDrawState->Transform(transform);
	_LocalToScreenTransform(transform);
	return transform;
}


SimpleTransform
Canvas::PenToLocalTransform() const GCC_2_NRV(transform)
{
#if __GNUC__ >= 3
	SimpleTransform transform;
#endif
	fDrawState->Transform(transform);
	return transform;
}


SimpleTransform
Canvas::ScreenToPenTransform() const GCC_2_NRV(transform)
{
#if __GNUC__ >= 3
	SimpleTransform transform;
#endif
	_ScreenToLocalTransform(transform);
	fDrawState->InverseTransform(transform);
	return transform;
}


void
Canvas::BlendLayer(Layer* layerPtr)
{
	BReference<Layer> layer(layerPtr, true);

	if (layer->Opacity() == 255) {
		layer->Play(this);
		return;
	}

	BReference <UtilityBitmap> layerBitmap(layer->RenderToBitmap(this), true);
	if (layerBitmap == NULL)
		return;

	BRect destination = layerBitmap->Bounds();
	destination.OffsetBy(layer->LeftTopOffset());
	LocalToScreenTransform().Apply(&destination);

	PushState();

	fDrawState->SetDrawingMode(B_OP_ALPHA);
	fDrawState->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	fDrawState->SetTransformEnabled(false);

	BReference<AlphaMask> mask(new(std::nothrow) UniformAlphaMask(layer->Opacity()), true);
	if (mask == NULL)
		return;

	SetAlphaMask(mask);
	ResyncDrawState();

	GetDrawingEngine()->DrawBitmap(layerBitmap, layerBitmap->Bounds(),
		destination, 0);

	fDrawState->SetTransformEnabled(true);

	PopState();
	ResyncDrawState();
}


// #pragma mark - OffscreenCanvas


OffscreenCanvas::OffscreenCanvas(DrawingEngine* engine,
		const DrawState& state, const IntRect& bounds)
	:
	Canvas(state),
	fDrawingEngine(engine),
	fBounds(bounds)
{
	ResyncDrawState();
}


OffscreenCanvas::~OffscreenCanvas()
{
}


void
OffscreenCanvas::ResyncDrawState()
{
	fDrawingEngine->SetDrawState(fDrawState.Get());
}


void
OffscreenCanvas::UpdateCurrentDrawingRegion()
{
	if (fDrawState->HasClipping()) {
		fDrawState->GetCombinedClippingRegion(&fCurrentDrawingRegion);
		fDrawingEngine->ConstrainClippingRegion(&fCurrentDrawingRegion);
	}
}


IntRect
OffscreenCanvas::Bounds() const
{
	return fBounds;
}
