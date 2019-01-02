/*
 * Copyright 2001-2018, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@mymail.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Joseph Groover <looncraz@looncraz.net>
 */

//!	Data classes for working with BView states and draw parameters

#include "DrawState.h"

#include <new>
#include <stdio.h>

#include <Region.h>
#include <ShapePrivate.h>

#include "AlphaMask.h"
#include "LinkReceiver.h"
#include "LinkSender.h"
#include "ServerProtocolStructs.h"


using std::nothrow;


DrawState::DrawState()
	:
	fOrigin(0.0f, 0.0f),
	fCombinedOrigin(0.0f, 0.0f),
	fScale(1.0f),
	fCombinedScale(1.0f),
	fTransform(),
	fCombinedTransform(),
	fClippingRegion(NULL),
	fAlphaMask(NULL),

	fHighColor((rgb_color){ 0, 0, 0, 255 }),
	fLowColor((rgb_color){ 255, 255, 255, 255 }),
	fWhichHighColor(B_NO_COLOR),
	fWhichLowColor(B_NO_COLOR),
	fWhichHighColorTint(B_NO_TINT),
	fWhichLowColorTint(B_NO_TINT),
	fPattern(kSolidHigh),

	fDrawingMode(B_OP_COPY),
	fAlphaSrcMode(B_PIXEL_ALPHA),
	fAlphaFncMode(B_ALPHA_OVERLAY),
	fDrawingModeLocked(false),

	fPenLocation(0.0f, 0.0f),
	fPenSize(1.0f),

	fFontAliasing(false),
	fSubPixelPrecise(false),
	fLineCapMode(B_BUTT_CAP),
	fLineJoinMode(B_MITER_JOIN),
	fMiterLimit(B_DEFAULT_MITER_LIMIT),
	fFillRule(B_NONZERO),
	fPreviousState(NULL)
{
	fUnscaledFontSize = fFont.Size();
}


DrawState::DrawState(const DrawState& other)
	:
	fOrigin(other.fOrigin),
	fCombinedOrigin(other.fCombinedOrigin),
	fScale(other.fScale),
	fCombinedScale(other.fCombinedScale),
	fTransform(other.fTransform),
	fCombinedTransform(other.fCombinedTransform),
	fClippingRegion(NULL),
	fAlphaMask(NULL),

	fHighColor(other.fHighColor),
	fLowColor(other.fLowColor),
	fWhichHighColor(other.fWhichHighColor),
	fWhichLowColor(other.fWhichLowColor),
	fWhichHighColorTint(other.fWhichHighColorTint),
	fWhichLowColorTint(other.fWhichLowColorTint),
	fPattern(other.fPattern),

	fDrawingMode(other.fDrawingMode),
	fAlphaSrcMode(other.fAlphaSrcMode),
	fAlphaFncMode(other.fAlphaFncMode),
	fDrawingModeLocked(other.fDrawingModeLocked),

	fPenLocation(other.fPenLocation),
	fPenSize(other.fPenSize),

	fFont(other.fFont),
	fFontAliasing(other.fFontAliasing),

	fSubPixelPrecise(other.fSubPixelPrecise),

	fLineCapMode(other.fLineCapMode),
	fLineJoinMode(other.fLineJoinMode),
	fMiterLimit(other.fMiterLimit),
	fFillRule(other.fFillRule),

	// Since fScale is reset to 1.0, the unscaled
	// font size is the current size of the font
	// (which is from->fUnscaledFontSize * from->fCombinedScale)
	fUnscaledFontSize(other.fUnscaledFontSize),
	fPreviousState(NULL)
{
}


DrawState::~DrawState()
{
	delete fClippingRegion;
	delete fPreviousState;
}


DrawState*
DrawState::PushState()
{
	DrawState* next = new (nothrow) DrawState(*this);

	if (next != NULL) {
		// Prepare state as derived from this state
		next->fOrigin = BPoint(0.0, 0.0);
		next->fScale = 1.0;
		next->fTransform.Reset();
		next->fPreviousState = this;
		next->SetAlphaMask(fAlphaMask);
	}

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


uint16
DrawState::ReadFontFromLink(BPrivate::LinkReceiver& link)
{
	uint16 mask;
	link.Read<uint16>(&mask);

	if ((mask & B_FONT_FAMILY_AND_STYLE) != 0) {
		uint32 fontID;
		link.Read<uint32>(&fontID);
		fFont.SetFamilyAndStyle(fontID);
	}

	if ((mask & B_FONT_SIZE) != 0) {
		float size;
		link.Read<float>(&size);
		fUnscaledFontSize = size;
		fFont.SetSize(fUnscaledFontSize * fCombinedScale);
	}

	if ((mask & B_FONT_SHEAR) != 0) {
		float shear;
		link.Read<float>(&shear);
		fFont.SetShear(shear);
	}

	if ((mask & B_FONT_ROTATION) != 0) {
		float rotation;
		link.Read<float>(&rotation);
		fFont.SetRotation(rotation);
	}

	if ((mask & B_FONT_FALSE_BOLD_WIDTH) != 0) {
		float falseBoldWidth;
		link.Read<float>(&falseBoldWidth);
		fFont.SetFalseBoldWidth(falseBoldWidth);
	}

	if ((mask & B_FONT_SPACING) != 0) {
		uint8 spacing;
		link.Read<uint8>(&spacing);
		fFont.SetSpacing(spacing);
	}

	if ((mask & B_FONT_ENCODING) != 0) {
		uint8 encoding;
		link.Read<uint8>(&encoding);
		fFont.SetEncoding(encoding);
	}

	if ((mask & B_FONT_FACE) != 0) {
		uint16 face;
		link.Read<uint16>(&face);
		fFont.SetFace(face);
	}

	if ((mask & B_FONT_FLAGS) != 0) {
		uint32 flags;
		link.Read<uint32>(&flags);
		fFont.SetFlags(flags);
	}

	return mask;
}


void
DrawState::ReadFromLink(BPrivate::LinkReceiver& link)
{
	ViewSetStateInfo info;

	link.Read<ViewSetStateInfo>(&info);

	// BAffineTransform is transmitted as a double array
	double transform[6];
	link.Read<double[6]>(&transform);
	if (fTransform.Unflatten(B_AFFINE_TRANSFORM_TYPE, transform,
		sizeof(transform)) != B_OK) {
		return;
	}

	fPenLocation = info.penLocation;
	fPenSize = info.penSize;
	fHighColor = info.highColor;
	fLowColor = info.lowColor;
	fWhichHighColor = info.whichHighColor;
	fWhichLowColor = info.whichLowColor;
	fWhichHighColorTint = info.whichHighColorTint;
	fWhichLowColorTint = info.whichLowColorTint;
	fPattern = info.pattern;
	fDrawingMode = info.drawingMode;
	fOrigin = info.origin;
	fScale = info.scale;
	fLineJoinMode = info.lineJoin;
	fLineCapMode = info.lineCap;
	fMiterLimit = info.miterLimit;
	fFillRule = info.fillRule;
	fAlphaSrcMode = info.alphaSourceMode;
	fAlphaFncMode = info.alphaFunctionMode;
	fFontAliasing = info.fontAntialiasing;

	if (fPreviousState != NULL) {
		fCombinedOrigin = fPreviousState->fCombinedOrigin + fOrigin;
		fCombinedScale = fPreviousState->fCombinedScale * fScale;
		fCombinedTransform = fPreviousState->fCombinedTransform * fTransform;
	} else {
		fCombinedOrigin = fOrigin;
		fCombinedScale = fScale;
		fCombinedTransform = fTransform;
	}


	// read clipping
	// TODO: This could be optimized, but the user clipping regions are rarely
	// used, so it's low priority...
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
	// Attach font state
	ViewGetStateInfo info;
	info.fontID = fFont.GetFamilyAndStyle();
	info.fontSize = fFont.Size();
	info.fontShear = fFont.Shear();
	info.fontRotation = fFont.Rotation();
	info.fontFalseBoldWidth = fFont.FalseBoldWidth();
	info.fontSpacing = fFont.Spacing();
	info.fontEncoding = fFont.Encoding();
	info.fontFace = fFont.Face();
	info.fontFlags = fFont.Flags();

	// Attach view state
	info.viewStateInfo.penLocation = fPenLocation;
	info.viewStateInfo.penSize = fPenSize;
	info.viewStateInfo.highColor = fHighColor;
	info.viewStateInfo.lowColor = fLowColor;
	info.viewStateInfo.whichHighColor = fWhichHighColor;
	info.viewStateInfo.whichLowColor = fWhichLowColor;
	info.viewStateInfo.whichHighColorTint = fWhichHighColorTint;
	info.viewStateInfo.whichLowColorTint = fWhichLowColorTint;
	info.viewStateInfo.pattern = (::pattern)fPattern.GetPattern();
	info.viewStateInfo.drawingMode = fDrawingMode;
	info.viewStateInfo.origin = fOrigin;
	info.viewStateInfo.scale = fScale;
	info.viewStateInfo.lineJoin = fLineJoinMode;
	info.viewStateInfo.lineCap = fLineCapMode;
	info.viewStateInfo.miterLimit = fMiterLimit;
	info.viewStateInfo.fillRule = fFillRule;
	info.viewStateInfo.alphaSourceMode = fAlphaSrcMode;
	info.viewStateInfo.alphaFunctionMode = fAlphaFncMode;
	info.viewStateInfo.fontAntialiasing = fFontAliasing;


	link.Attach<ViewGetStateInfo>(info);

	// BAffineTransform is transmitted as a double array
	double transform[6];
	if (fTransform.Flatten(transform, sizeof(transform)) != B_OK)
		return;
	link.Attach<double[6]>(transform);

	// TODO: Could be optimized, but is low prio, since most views do not
	// use a custom clipping region...
	if (fClippingRegion != NULL) {
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
DrawState::SetOrigin(BPoint origin)
{
	fOrigin = origin;

	// NOTE: the origins of earlier states are never expected to
	// change, only the topmost state ever changes
	if (fPreviousState != NULL) {
		fCombinedOrigin.x = fPreviousState->fCombinedOrigin.x
			+ fOrigin.x * fPreviousState->fCombinedScale;
		fCombinedOrigin.y = fPreviousState->fCombinedOrigin.y
			+ fOrigin.y * fPreviousState->fCombinedScale;
	} else {
		fCombinedOrigin = fOrigin;
	}
}


void
DrawState::SetScale(float scale)
{
	if (fScale == scale)
		return;

	fScale = scale;

	// NOTE: the scales of earlier states are never expected to
	// change, only the topmost state ever changes
	if (fPreviousState != NULL)
		fCombinedScale = fPreviousState->fCombinedScale * fScale;
	else
		fCombinedScale = fScale;

	// update font size
	// NOTE: This is what makes the call potentially expensive,
	// hence the introductory check
	fFont.SetSize(fUnscaledFontSize * fCombinedScale);
}


void
DrawState::SetTransform(BAffineTransform transform)
{
	if (fTransform == transform)
		return;

	fTransform = transform;

	// NOTE: the transforms of earlier states are never expected to
	// change, only the topmost state ever changes
	if (fPreviousState != NULL)
		fCombinedTransform = fPreviousState->fCombinedTransform * fTransform;
	else
		fCombinedTransform = fTransform;
}


/* Can be used to temporarily disable all BAffineTransforms in the state
   stack, and later reenable them.
*/
void
DrawState::SetTransformEnabled(bool enabled)
{
	if (enabled) {
		BAffineTransform temp = fTransform;
		SetTransform(BAffineTransform());
		SetTransform(temp);
	}
	else
		fCombinedTransform = BAffineTransform();
}


DrawState*
DrawState::Squash() const
{
	DrawState* const squashedState = new(nothrow) DrawState(*this);
	return squashedState->PushState();
}


void
DrawState::SetClippingRegion(const BRegion* region)
{
	if (region) {
		if (fClippingRegion != NULL)
			*fClippingRegion = *region;
		else
			fClippingRegion = new(nothrow) BRegion(*region);
	} else {
		delete fClippingRegion;
		fClippingRegion = NULL;
	}
}


bool
DrawState::HasClipping() const
{
	if (fClippingRegion != NULL)
		return true;
	if (fPreviousState != NULL)
		return fPreviousState->HasClipping();
	return false;
}


bool
DrawState::HasAdditionalClipping() const
{
	return fClippingRegion != NULL;
}


bool
DrawState::GetCombinedClippingRegion(BRegion* region) const
{
	if (fClippingRegion != NULL) {
		BRegion localTransformedClipping(*fClippingRegion);
		SimpleTransform penTransform;
		Transform(penTransform);
		penTransform.Apply(&localTransformedClipping);
		if (fPreviousState != NULL
			&& fPreviousState->GetCombinedClippingRegion(region)) {
			localTransformedClipping.IntersectWith(region);
		}
		*region = localTransformedClipping;
		return true;
	} else {
		if (fPreviousState != NULL)
			return fPreviousState->GetCombinedClippingRegion(region);
	}
	return false;
}


bool
DrawState::ClipToRect(BRect rect, bool inverse)
{
	if (!rect.IsValid())
		return false;

	if (!fCombinedTransform.IsIdentity()) {
		if (fCombinedTransform.IsDilation()) {
			BPoint points[2] = { rect.LeftTop(), rect.RightBottom() };
			fCombinedTransform.Apply(&points[0], 2);
			rect.Set(points[0].x, points[0].y, points[1].x, points[1].y);
		} else {
			uint32 ops[] = {
				OP_MOVETO | OP_LINETO | 3,
				OP_CLOSE
			};
			BPoint points[4] = {
				BPoint(rect.left,  rect.top),
				BPoint(rect.right, rect.top),
				BPoint(rect.right, rect.bottom),
				BPoint(rect.left,  rect.bottom)
			};
			shape_data rectShape;
			rectShape.opList = &ops[0];
			rectShape.opCount = 2;
			rectShape.opSize = sizeof(uint32) * 2;
			rectShape.ptList = &points[0];
			rectShape.ptCount = 4;
			rectShape.ptSize = sizeof(BPoint) * 4;

			ClipToShape(&rectShape, inverse);
			return true;
		}
	}

	if (inverse) {
		if (fClippingRegion == NULL) {
			fClippingRegion = new(nothrow) BRegion(BRect(
				-(1 << 16), -(1 << 16), (1 << 16), (1 << 16)));
				// TODO: we should have a definition for a rect (or region)
				// with "infinite" area. For now, this region size should do...
		}
		fClippingRegion->Exclude(rect);
	} else {
		if (fClippingRegion == NULL)
			fClippingRegion = new(nothrow) BRegion(rect);
		else {
			BRegion rectRegion(rect);
			fClippingRegion->IntersectWith(&rectRegion);
		}
	}

	return false;
}


void
DrawState::ClipToShape(shape_data* shape, bool inverse)
{
	if (shape->ptCount == 0)
		return;

	if (!fCombinedTransform.IsIdentity())
		fCombinedTransform.Apply(shape->ptList, shape->ptCount);

	AlphaMask* const mask = ShapeAlphaMask::Create(GetAlphaMask(), *shape,
		BPoint(0, 0), inverse);

	SetAlphaMask(mask);
	if (mask != NULL)
		mask->ReleaseReference();
}


void
DrawState::SetAlphaMask(AlphaMask* mask)
{
	// NOTE: In BeOS, it wasn't possible to clip to a BPicture and keep
	// regular custom clipping to a BRegion at the same time.
	fAlphaMask.SetTo(mask);
}


AlphaMask*
DrawState::GetAlphaMask() const
{
	return fAlphaMask.Get();
}


// #pragma mark -


void
DrawState::Transform(SimpleTransform& transform) const
{
	transform.AddOffset(fCombinedOrigin.x, fCombinedOrigin.y);
	transform.SetScale(fCombinedScale);
}


void
DrawState::InverseTransform(SimpleTransform& transform) const
{
	transform.AddOffset(-fCombinedOrigin.x, -fCombinedOrigin.y);
	if (fCombinedScale != 0.0)
		transform.SetScale(1.0 / fCombinedScale);
}


// #pragma mark -


void
DrawState::SetHighColor(rgb_color color)
{
	fHighColor = color;
}


void
DrawState::SetLowColor(rgb_color color)
{
	fLowColor = color;
}


void
DrawState::SetHighUIColor(color_which which, float tint)
{
	fWhichHighColor = which;
	fWhichHighColorTint = tint;
}


color_which
DrawState::HighUIColor(float* tint) const
{
	if (tint != NULL)
		*tint = fWhichHighColorTint;

	return fWhichHighColor;
}


void
DrawState::SetLowUIColor(color_which which, float tint)
{
	fWhichLowColor = which;
	fWhichLowColorTint = tint;
}


color_which
DrawState::LowUIColor(float* tint) const
{
	if (tint != NULL)
		*tint = fWhichLowColorTint;

	return fWhichLowColor;
}


void
DrawState::SetPattern(const Pattern& pattern)
{
	fPattern = pattern;
}


bool
DrawState::SetDrawingMode(drawing_mode mode)
{
	if (!fDrawingModeLocked) {
		fDrawingMode = mode;
		return true;
	}
	return false;
}


bool
DrawState::SetBlendingMode(source_alpha srcMode, alpha_function fncMode)
{
	if (!fDrawingModeLocked) {
		fAlphaSrcMode = srcMode;
		fAlphaFncMode = fncMode;
		return true;
	}
	return false;
}


void
DrawState::SetDrawingModeLocked(bool locked)
{
	fDrawingModeLocked = locked;
}



void
DrawState::SetPenLocation(BPoint location)
{
	fPenLocation = location;
}


BPoint
DrawState::PenLocation() const
{
	return fPenLocation;
}


void
DrawState::SetPenSize(float size)
{
	fPenSize = size;
}


//! returns the scaled pen size
float
DrawState::PenSize() const
{
	float penSize = fPenSize * fCombinedScale;
	// NOTE: As documented in the BeBook,
	// pen size is never smaller than 1.0.
	// This is supposed to be the smallest
	// possible device size.
	if (penSize < 1.0)
		penSize = 1.0;
	return penSize;
}


//! returns the unscaled pen size
float
DrawState::UnscaledPenSize() const
{
	// NOTE: As documented in the BeBook,
	// pen size is never smaller than 1.0.
	// This is supposed to be the smallest
	// possible device size.
	return max_c(fPenSize, 1.0);
}


//! sets the font to be already scaled by fScale
void
DrawState::SetFont(const ServerFont& font, uint32 flags)
{
	if (flags == B_FONT_ALL) {
		fFont = font;
		fUnscaledFontSize = font.Size();
		fFont.SetSize(fUnscaledFontSize * fCombinedScale);
	} else {
		// family & style
		if ((flags & B_FONT_FAMILY_AND_STYLE) != 0)
			fFont.SetFamilyAndStyle(font.GetFamilyAndStyle());
		// size
		if ((flags & B_FONT_SIZE) != 0) {
			fUnscaledFontSize = font.Size();
			fFont.SetSize(fUnscaledFontSize * fCombinedScale);
		}
		// shear
		if ((flags & B_FONT_SHEAR) != 0)
			fFont.SetShear(font.Shear());
		// rotation
		if ((flags & B_FONT_ROTATION) != 0)
			fFont.SetRotation(font.Rotation());
		// spacing
		if ((flags & B_FONT_SPACING) != 0)
			fFont.SetSpacing(font.Spacing());
		// encoding
		if ((flags & B_FONT_ENCODING) != 0)
			fFont.SetEncoding(font.Encoding());
		// face
		if ((flags & B_FONT_FACE) != 0)
			fFont.SetFace(font.Face());
		// flags
		if ((flags & B_FONT_FLAGS) != 0)
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
DrawState::SetFillRule(int32 fillRule)
{
	fFillRule = fillRule;
}


void
DrawState::PrintToStream() const
{
	printf("\t Origin: (%.1f, %.1f)\n", fOrigin.x, fOrigin.y);
	printf("\t Scale: %.2f\n", fScale);
	printf("\t Transform: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n",
		fTransform.sx, fTransform.shy, fTransform.shx,
		fTransform.sy, fTransform.tx, fTransform.ty);

	printf("\t Pen Location and Size: (%.1f, %.1f) - %.2f (%.2f)\n",
		   fPenLocation.x, fPenLocation.y, PenSize(), fPenSize);

	printf("\t HighColor: r=%d g=%d b=%d a=%d\n",
		fHighColor.red, fHighColor.green, fHighColor.blue, fHighColor.alpha);
	printf("\t LowColor: r=%d g=%d b=%d a=%d\n",
		fLowColor.red, fLowColor.green, fLowColor.blue, fLowColor.alpha);
	printf("\t WhichHighColor: %i\n", fWhichHighColor);
	printf("\t WhichLowColor: %i\n", fWhichLowColor);
	printf("\t WhichHighColorTint: %.3f\n", fWhichHighColorTint);
	printf("\t WhichLowColorTint: %.3f\n", fWhichLowColorTint);
	printf("\t Pattern: %" B_PRIu64 "\n", fPattern.GetInt64());

	printf("\t DrawMode: %" B_PRIu32 "\n", (uint32)fDrawingMode);
	printf("\t AlphaSrcMode: %" B_PRId32 "\t AlphaFncMode: %" B_PRId32 "\n",
		   (int32)fAlphaSrcMode, (int32)fAlphaFncMode);

	printf("\t LineCap: %d\t LineJoin: %d\t MiterLimit: %.2f\n",
		   (int16)fLineCapMode, (int16)fLineJoinMode, fMiterLimit);

	if (fClippingRegion != NULL)
		fClippingRegion->PrintToStream();

	printf("\t ===== Font Data =====\n");
	printf("\t Style: CURRENTLY NOT SET\n"); // ???
	printf("\t Size: %.1f (%.1f)\n", fFont.Size(), fUnscaledFontSize);
	printf("\t Shear: %.2f\n", fFont.Shear());
	printf("\t Rotation: %.2f\n", fFont.Rotation());
	printf("\t Spacing: %" B_PRId32 "\n", fFont.Spacing());
	printf("\t Encoding: %" B_PRId32 "\n", fFont.Encoding());
	printf("\t Face: %d\n", fFont.Face());
	printf("\t Flags: %" B_PRIu32 "\n", fFont.Flags());
}

