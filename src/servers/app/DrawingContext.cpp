/*
 * Copyright (c) 2001-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk
 */


#include "DrawingContext.h"

#include <new>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <Region.h>

#include "DrawingEngine.h"
#include "DrawState.h"


DrawingContext::DrawingContext()
	:
	fDrawState(new(std::nothrow) DrawState())
{
}


DrawingContext::DrawingContext(const DrawState& state)
	:
	fDrawState(new(std::nothrow) DrawState(state))
{
}


DrawingContext::~DrawingContext()
{
}


status_t
DrawingContext::InitCheck() const
{
	if (fDrawState == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
DrawingContext::PushState()
{
	DrawState* newState = fDrawState->PushState();
	if (newState)
		fDrawState = newState;
}


void
DrawingContext::PopState()
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
DrawingContext::SetDrawingOrigin(BPoint origin)
{
	fDrawState->SetOrigin(origin);

	// rebuild clipping
	if (fDrawState->HasClipping())
		RebuildClipping(false);
}


BPoint
DrawingContext::DrawingOrigin() const
{
	BPoint origin(fDrawState->Origin());
	float scale = Scale();

	origin.x *= scale;
	origin.y *= scale;

	return origin;
}


void
DrawingContext::SetScale(float scale)
{
	fDrawState->SetScale(scale);

	// rebuild clipping
	if (fDrawState->HasClipping())
		RebuildClipping(false);
}


float
DrawingContext::Scale() const
{
	return fDrawState->Scale();
}


void
DrawingContext::SetUserClipping(const BRegion* region)
{
	fDrawState->SetClippingRegion(region);

	// rebuild clipping (for just this context)
	RebuildClipping(false);
}


void
DrawingContext::SetAlphaMask(AlphaMask* mask)
{
	fDrawState->SetAlphaMask(mask);
}


AlphaMask*
DrawingContext::GetAlphaMask() const
{
	return fDrawState->GetAlphaMask();
}


//! converts a point from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BPoint* point) const
{
	fDrawState->Transform(point);
	// NOTE: from here on, don't use the
	// "*ForDrawing()" versions of the parent!
	ConvertToScreen(point);
}


//! converts a rect from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BRect* rect) const
{
	fDrawState->Transform(rect);
	// NOTE: from here on, don't use the
	// "*ForDrawing()" versions of the parent!
	ConvertToScreen(rect);
}


//! converts a region from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BRegion* region) const
{
	fDrawState->Transform(region);
	// NOTE: from here on, don't use the
	// "*ForDrawing()" versions of the parent!
	ConvertToScreen(region);
}


//! converts a gradient from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BGradient* gradient) const
{
	switch(gradient->GetType()) {
		case BGradient::TYPE_LINEAR:
		{
			BGradientLinear* linear = (BGradientLinear*) gradient;
			BPoint start = linear->Start();
			BPoint end = linear->End();
			fDrawState->Transform(&start);
			ConvertToScreen(&start);
			fDrawState->Transform(&end);
			ConvertToScreen(&end);
			linear->SetStart(start);
			linear->SetEnd(end);
			linear->SortColorStopsByOffset();
			break;
		}
		case BGradient::TYPE_RADIAL:
		{
			BGradientRadial* radial = (BGradientRadial*) gradient;
			BPoint center = radial->Center();
			fDrawState->Transform(&center);
			ConvertToScreen(&center);
			radial->SetCenter(center);
			radial->SortColorStopsByOffset();
			break;
		}
		case BGradient::TYPE_RADIAL_FOCUS:
		{
			BGradientRadialFocus* radialFocus = (BGradientRadialFocus*) gradient;
			BPoint center = radialFocus->Center();
			BPoint focal = radialFocus->Focal();
			fDrawState->Transform(&center);
			ConvertToScreen(&center);
			fDrawState->Transform(&focal);
			ConvertToScreen(&focal);
			radialFocus->SetCenter(center);
			radialFocus->SetFocal(focal);
			radialFocus->SortColorStopsByOffset();
			break;
		}
		case BGradient::TYPE_DIAMOND:
		{
			BGradientDiamond* diamond = (BGradientDiamond*) gradient;
			BPoint center = diamond->Center();
			fDrawState->Transform(&center);
			ConvertToScreen(&center);
			diamond->SetCenter(center);
			diamond->SortColorStopsByOffset();
			break;
		}
		case BGradient::TYPE_CONIC:
		{
			BGradientConic* conic = (BGradientConic*) gradient;
			BPoint center = conic->Center();
			fDrawState->Transform(&center);
			ConvertToScreen(&center);
			conic->SetCenter(center);
			conic->SortColorStopsByOffset();
			break;
		}
		case BGradient::TYPE_NONE:
		{
			break;
		}
	}
}


//! converts points from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BPoint* dst, const BPoint* src, int32 num) const
{
	// TODO: optimize this, it should be smarter
	while (num--) {
		*dst = *src;
		fDrawState->Transform(dst);
		// NOTE: from here on, don't use the
		// "*ForDrawing()" versions of the parent!
		ConvertToScreen(dst);
		src++;
		dst++;
	}
}


//! converts rects from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BRect* dst, const BRect* src, int32 num) const
{
	// TODO: optimize this, it should be smarter
	while (num--) {
		*dst = *src;
		fDrawState->Transform(dst);
		// NOTE: from here on, don't use the
		// "*ForDrawing()" versions of the parent!
		ConvertToScreen(dst);
		src++;
		dst++;
	}
}


//! converts regions from local *drawing* to screen coordinate system
void
DrawingContext::ConvertToScreenForDrawing(BRegion* dst, const BRegion* src, int32 num) const
{
	// TODO: optimize this, it should be smarter
	while (num--) {
		*dst = *src;
		fDrawState->Transform(dst);
		// NOTE: from here on, don't use the
		// "*ForDrawing()" versions of the parent!
		ConvertToScreen(dst);
		src++;
		dst++;
	}
}


//! converts a point from screen to local coordinate system
void
DrawingContext::ConvertFromScreenForDrawing(BPoint* point) const
{
	ConvertFromScreen(point);
	fDrawState->InverseTransform(point);
}


// #pragma mark - OffscreenContext


OffscreenContext::OffscreenContext(DrawingEngine* engine,
		const DrawState& state)
	:
	DrawingContext(state),
	fDrawingEngine(engine)
{
	ResyncDrawState();
}


void
OffscreenContext::ResyncDrawState()
{
	fDrawingEngine->SetDrawState(fDrawState);
}

