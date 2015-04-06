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

#include "DrawingEngine.h"
#include "DrawState.h"


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
	if (fDrawState == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
Canvas::PushState()
{
	DrawState* newState = fDrawState->PushState();
	if (newState)
		fDrawState = newState;
}


void
Canvas::PopState()
{
	if (fDrawState->PreviousState() == NULL)
		return;

	bool rebuildClipping = fDrawState->HasAdditionalClipping();

	fDrawState = fDrawState->PopState();

	// rebuild clipping
	// (the clipping from the popped state is not effective anymore)
	if (rebuildClipping)
		RebuildClipping(false);
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
Canvas::ScreenToPenTransform() const GCC_2_NRV(transform)
{
#if __GNUC__ >= 3
	SimpleTransform transform;
#endif
	_ScreenToLocalTransform(transform);
	fDrawState->InverseTransform(transform);
	return transform;
}


// #pragma mark - OffscreenCanvas


OffscreenCanvas::OffscreenCanvas(DrawingEngine* engine,
		const DrawState& state)
	:
	Canvas(state),
	fDrawingEngine(engine)
{
	ResyncDrawState();
}


void
OffscreenCanvas::ResyncDrawState()
{
	fDrawingEngine->SetDrawState(fDrawState);
}

