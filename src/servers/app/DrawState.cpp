/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@mymail.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Data classes for working with BView states and draw parameters */


#include <stdio.h>

#include <Region.h>

#include "LinkReceiver.h"
#include "LinkSender.h"

#include "DrawState.h"


DrawState::DrawState()
	: fOrigin(0.0, 0.0),
	  fScale(1.0),
	  fClippingRegion(NULL),
	  fHighColor(0, 0, 0, 255),
	  fLowColor(255, 255, 255, 255),
	  fPattern(kSolidHigh),
	  fDrawingMode(B_OP_COPY),
	  fAlphaSrcMode(B_PIXEL_ALPHA),
	  fAlphaFncMode(B_ALPHA_OVERLAY),
	  fPenLocation(0.0, 0.0),
	  fPenSize(1.0),
	  fFontAliasing(false),
	  fSubPixelPrecise(false),
	  fLineCapMode(B_BUTT_CAP),
	  fLineJoinMode(B_MITER_JOIN),
	  fMiterLimit(B_DEFAULT_MITER_LIMIT),
	  fPreviousState(NULL)
{
	fUnscaledFontSize = fFont.Size();
}


DrawState::DrawState(const DrawState& from)
	: fClippingRegion(NULL)
{
	*this = from;
}


DrawState::~DrawState()
{
	delete fClippingRegion;
	delete fPreviousState;
}


DrawState&
DrawState::operator=(const DrawState& from)
{
	fOrigin	= from.fOrigin;
	fScale	= from.fScale;

	if (from.fClippingRegion) {
		if (fClippingRegion)
			*fClippingRegion = *from.fClippingRegion;
		else
			fClippingRegion = new BRegion(*from.fClippingRegion);
	} else {
		delete fClippingRegion;
		fClippingRegion = NULL;
	}

	fHighColor			= from.fHighColor;
	fLowColor			= from.fLowColor;
	fPattern			= from.fPattern;

	fDrawingMode		= from.fDrawingMode;
	fAlphaSrcMode		= from.fAlphaSrcMode;
	fAlphaFncMode		= from.fAlphaFncMode;

	fPenLocation		= from.fPenLocation;
	fPenSize			= from.fPenSize;

	fFont				= from.fFont;
	fFontAliasing		= from.fFontAliasing;

	fSubPixelPrecise	= from.fSubPixelPrecise;

	fLineCapMode		= from.fLineCapMode;
	fLineJoinMode		= from.fLineJoinMode;
	fMiterLimit			= from.fMiterLimit;

	// Since fScale is reset to 1.0, the unscaled
	// font size is the current size of the font
	// (which is from->fUnscaledFontSize * from->fScale)
	fUnscaledFontSize	= from.fUnscaledFontSize;
	fPreviousState		= from.fPreviousState;

	return *this;
}


DrawState*
DrawState::PushState()
{
	DrawState* next = new DrawState(*this);
		// this may throw an exception that our caller should handle

	next->fPreviousState = this;
	return next;
}


DrawState*
DrawState::PopState()
{
	DrawState* previous = PreviousState();

	fPreviousState = NULL;
	delete this;

	return previous;
}


void
DrawState::ReadFontFromLink(BPrivate::LinkReceiver& link)
{
	uint16 mask;
	link.Read<uint16>(&mask);

	if (mask & B_FONT_FAMILY_AND_STYLE) {
		uint32 fontID;
		link.Read<uint32>(&fontID);
		fFont.SetFamilyAndStyle(fontID);
	}

	if (mask & B_FONT_SIZE) {
		float size;
		link.Read<float>(&size);
		fFont.SetSize(size);
	}
	
	if (mask & B_FONT_SHEAR) {
		float shear;
		link.Read<float>(&shear);
		fFont.SetShear(shear);
	}

	if (mask & B_FONT_ROTATION) {
		float rotation;
		link.Read<float>(&rotation);
		fFont.SetRotation(rotation);
	}

	if (mask & B_FONT_SPACING) {
		uint8 spacing;
		link.Read<uint8>(&spacing);
		fFont.SetSpacing(spacing);
	}

	if (mask & B_FONT_ENCODING) {
		uint8 encoding;
		link.Read<uint8>((uint8*)&encoding);
		fFont.SetEncoding(encoding);
	}

	if (mask & B_FONT_FACE) {
		uint16 face;
		link.Read<uint16>(&face);
		fFont.SetFace(face);
	}

	if (mask & B_FONT_FLAGS) {
		uint32 flags;
		link.Read<uint32>(&flags);
		fFont.SetFlags(flags);
	}
}


void
DrawState::ReadFromLink(BPrivate::LinkReceiver& link)
{
	rgb_color highColor;
	rgb_color lowColor;
	pattern patt;

	link.Read<BPoint>(&fPenLocation);
	link.Read<float>(&fPenSize);
	link.Read(&highColor, sizeof(rgb_color));
	link.Read(&lowColor, sizeof(rgb_color));
	link.Read(&patt, sizeof(pattern));
	link.Read<int8>((int8*)&fDrawingMode);
	link.Read<BPoint>(&fOrigin);
	link.Read<int8>((int8*)&fLineJoinMode);
	link.Read<int8>((int8*)&fLineCapMode);
	link.Read<float>(&fMiterLimit);
	link.Read<int8>((int8*)&fAlphaSrcMode);
	link.Read<int8>((int8*)&fAlphaFncMode);
	link.Read<float>(&fScale);
	link.Read<bool>(&fFontAliasing);

	fHighColor = highColor;
	fLowColor = lowColor;
	fPattern = patt;

	// read clipping
	int32 clipRectCount;
	link.Read<int32>(&clipRectCount);

	if (clipRectCount >= 0) {
		BRegion region;
		BRect rect;
		for (int32 i = 0; i < clipRectCount; i++) {
			link.Read<BRect>(&rect);
			region.Include(rect);
		}
		SetClippingRegion(&region);
	} else {
		// No user clipping used
		SetClippingRegion(NULL);
	}
}


void
DrawState::WriteToLink(BPrivate::LinkSender& link) const
{
	rgb_color hc = fHighColor.GetColor32();
	rgb_color lc = fLowColor.GetColor32();
	
	// Attach font state
	link.Attach<uint32>(fFont.GetFamilyAndStyle());
	link.Attach<float>(fFont.Size());
	link.Attach<float>(fFont.Shear());
	link.Attach<float>(fFont.Rotation());
	link.Attach<uint8>(fFont.Spacing());
	link.Attach<uint8>(fFont.Encoding());
	link.Attach<uint16>(fFont.Face());
	link.Attach<uint32>(fFont.Flags());
	
	// Attach view state
	link.Attach<BPoint>(fPenLocation);
	link.Attach<float>(fPenSize);
	link.Attach(&hc, sizeof(rgb_color));
	link.Attach(&lc, sizeof(rgb_color));
	link.Attach<uint64>(fPattern.GetInt64());
	link.Attach<BPoint>(fOrigin);
	link.Attach<uint8>((uint8)fDrawingMode);
	link.Attach<uint8>((uint8)fLineCapMode);
	link.Attach<uint8>((uint8)fLineJoinMode);
	link.Attach<float>(fMiterLimit);
	link.Attach<uint8>((uint8)fAlphaSrcMode);
	link.Attach<uint8>((uint8)fAlphaFncMode);
	link.Attach<float>(fScale);
	link.Attach<bool>(fFontAliasing);


	if (fClippingRegion) {
		int32 clippingRectCount = fClippingRegion->CountRects();
		link.Attach<int32>(clippingRectCount);
		for (int i = 0; i < clippingRectCount; i++)
			link.Attach<BRect>(fClippingRegion->RectAt(i));
	} else {
		// no client clipping
		link.Attach<int32>(-1);
	}
}


void
DrawState::SetOrigin(const BPoint& origin)
{
	fOrigin = fPreviousState ? fPreviousState->fOrigin + origin : origin;
}


void
DrawState::OffsetOrigin(const BPoint& offset)
{
	fOrigin += offset;
}


void
DrawState::SetScale(float scale)
{
	// the scale is multiplied with the scale of the previous state if any
	float localScale = fScale;
	if (PreviousState() != NULL)
		localScale /= PreviousState()->Scale();

	if (localScale != scale) {
		fScale = scale;
		if (PreviousState() != NULL)
			fScale *= PreviousState()->Scale();

		// update font size
		// (pen size is currently calulated on the fly)
		fFont.SetSize(fUnscaledFontSize * fScale);
	}
}

// Transform
void
DrawState::Transform(float* x, float* y) const
{
	*x += fOrigin.x;
	*y += fOrigin.y;
	*x *= fScale;
	*y *= fScale;
}

// InverseTransform
void
DrawState::InverseTransform(float* x, float* y) const
{
	// TODO: watch out for fScale = 0?
	*x /= fScale;
	*y /= fScale;
	*x -= fOrigin.x;
	*y -= fOrigin.y;
}

// Transform
void
DrawState::Transform(BPoint* point) const
{
	Transform(&(point->x), &(point->y));
}

// Transform
void
DrawState::Transform(BRect* rect) const
{
	Transform(&(rect->left), &(rect->top));
	Transform(&(rect->right), &(rect->bottom));
}

// Transform
void
DrawState::Transform(BRegion* region) const
{
	if (fScale == 1.0) {
		if (fOrigin.x != 0.0 || fOrigin.y != 0.0)
			region->OffsetBy((int32)fOrigin.x, (int32)fOrigin.y);
	} else {
		// TODO: optimize some more
		BRegion converted;
		int32 count = region->CountRects();
		for (int32 i = 0; i < count; i++) {
			BRect r = region->RectAt(i);
			BPoint lt(r.LeftTop());
			BPoint rb(r.RightBottom());
			// offset to bottom right corner of pixel before transformation
			rb.x++;
			rb.y++;
			// apply transformation
			Transform(&lt.x, &lt.y);
			Transform(&rb.x, &rb.y);
			// reset bottom right to pixel "index"
			rb.x++;
			rb.y++;
			// add rect to converted region
			// NOTE/TODO: the rect would not have to go
			// through the whole intersection test process,
			// it is guaranteed not to overlap with any rect
			// already contained in the region
			converted.Include(BRect(lt, rb));
		}
		*region = converted;
	}
}


void
DrawState::InverseTransform(BPoint* point) const
{
	InverseTransform(&(point->x), &(point->y));
}


void
DrawState::SetClippingRegion(const BRegion* region)
{
	// reset clipping to that of previous state
	// (that's the starting point)
	if (PreviousState() != NULL && PreviousState()->ClippingRegion()) {
		if (fClippingRegion)
			*fClippingRegion = *(PreviousState()->ClippingRegion());
		else
			fClippingRegion = new BRegion(*(PreviousState()->ClippingRegion()));
	} else {
		delete fClippingRegion;
		fClippingRegion = NULL;
	}

	// intersect with the clipping from the passed region
	// (even if it is empty)
	// passing NULL unsets this states additional region,
	// it will then be the region of the previous state
	if (region) {
		if (fClippingRegion)
			fClippingRegion->IntersectWith(region);
		else 
			fClippingRegion = new BRegion(*region);
	}
}


void
DrawState::SetHighColor(const RGBColor& color)
{
	fHighColor = color;
}


void
DrawState::SetLowColor(const RGBColor& color)
{
	fLowColor = color;
}


void
DrawState::SetPattern(const Pattern& pattern)
{
	fPattern = pattern;
}


void
DrawState::SetDrawingMode(drawing_mode mode)
{
	fDrawingMode = mode;
}


void
DrawState::SetBlendingMode(source_alpha srcMode, alpha_function fncMode)
{
	fAlphaSrcMode = srcMode;
	fAlphaFncMode = fncMode;
}


void
DrawState::SetPenLocation(const BPoint& location)
{
	// TODO: Needs to be in local coordinate system!
	// There is going to be some work involved in
	// other parts of app_server...
	fPenLocation = location;
}


const BPoint&
DrawState::PenLocation() const
{
	// TODO: See above
	return fPenLocation;
}


void
DrawState::SetPenSize(float size)
{
	// NOTE: since pensize is calculated on the fly,
	// it is ok to set it here regardless of previous state
	fPenSize = size;
}


//! returns the scaled pen size
float
DrawState::PenSize() const
{
	float penSize = fPenSize * fScale;
	// NOTE: As documented in the BeBook,
	// pen size is never smaller than 1.0.
	// This is supposed to be the smallest
	// possible device size.
	if (penSize < 1.0)
		penSize = 1.0;
	return penSize;
}


//! sets the font to be already scaled by fScale
void
DrawState::SetFont(const ServerFont& font, uint32 flags)
{
	if (flags == B_FONT_ALL) {
		fFont = font;
		fUnscaledFontSize = font.Size();
		fFont.SetSize(fUnscaledFontSize * fScale);
	} else {
		// family & style
		if (flags & B_FONT_FAMILY_AND_STYLE)
			fFont.SetFamilyAndStyle(font.GetFamilyAndStyle());
		// size
		if (flags & B_FONT_SIZE) {
			fUnscaledFontSize = font.Size();
			fFont.SetSize(fUnscaledFontSize * fScale);
		}
		// shear
		if (flags & B_FONT_SHEAR)
			fFont.SetShear(font.Shear());
		// rotation
		if (flags & B_FONT_ROTATION)
			fFont.SetRotation(font.Rotation());
		// spacing
		if (flags & B_FONT_SPACING)
			fFont.SetSpacing(font.Spacing());
		// encoding
		if (flags & B_FONT_ENCODING)
			fFont.SetEncoding(font.Encoding());
		// face
		if (flags & B_FONT_FACE)
			fFont.SetFace(font.Face());
		// flags
		if (flags & B_FONT_FLAGS)
			fFont.SetFlags(font.Flags());
	}
}


void
DrawState::SetForceFontAliasing(bool aliasing)
{
	fFontAliasing = aliasing;
}


void
DrawState::SetSubPixelPrecise(bool precise)
{
	fSubPixelPrecise = precise;
}


void
DrawState::SetLineCapMode(cap_mode mode)
{
	fLineCapMode = mode;
}


void
DrawState::SetLineJoinMode(join_mode mode)
{
	fLineJoinMode = mode;
}


void
DrawState::SetMiterLimit(float limit)
{
	fMiterLimit = limit;
}


void
DrawState::PrintToStream() const
{
	printf("\t Origin: (%.1f, %.1f)\n", fOrigin.x, fOrigin.y);
	printf("\t Scale: %.2f\n", fScale);

	printf("\t Pen Location and Size: (%.1f, %.1f) - %.2f (%.2f)\n",
		   fPenLocation.x, fPenLocation.y, PenSize(), fPenSize);

	printf("\t HighColor: "); fHighColor.PrintToStream();
	printf("\t LowColor: "); fLowColor.PrintToStream();
	printf("\t Pattern: %llu\n", fPattern.GetInt64());

	printf("\t DrawMode: %lu\n", (uint32)fDrawingMode);
	printf("\t AlphaSrcMode: %ld\t AlphaFncMode: %ld\n",
		   (int32)fAlphaSrcMode, (int32)fAlphaFncMode);

	printf("\t LineCap: %d\t LineJoin: %d\t MiterLimit: %.2f\n",
		   (int16)fLineCapMode, (int16)fLineJoinMode, fMiterLimit);

	if (fClippingRegion)
		fClippingRegion->PrintToStream();

	printf("\t ===== Font Data =====\n");
	printf("\t Style: CURRENTLY NOT SET\n"); // ???
	printf("\t Size: %.1f (%.1f)\n", fFont.Size(), fUnscaledFontSize);
	printf("\t Shear: %.2f\n", fFont.Shear());
	printf("\t Rotation: %.2f\n", fFont.Rotation());
	printf("\t Spacing: %ld\n", fFont.Spacing());
	printf("\t Encoding: %ld\n", fFont.Encoding());
	printf("\t Face: %d\n", fFont.Face());
	printf("\t Flags: %lu\n", fFont.Flags());
}

