/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
 * rendering pipe-lines for stroke, fills, bitmap and text rendering.
 */


#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Region.h>
#include <String.h>

#include <agg_bezier_arc.h>
#include <agg_bounding_rect.h>
#include <agg_conv_clip_polygon.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>
#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_rounded_rect.h>
#include <agg_span_image_filter_rgba32.h>
#include <agg_span_interpolator_linear.h>

#include "drawing_support.h"

#include "DrawState.h"

#include "AGGTextRenderer.h"
#include "DrawingMode.h"
#include "PatternHandler.h"
#include "RenderingBuffer.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "SystemPalette.h"

#include "Painter.h"


#if ALIASED_DRAWING
// in this case, we _cannot_ use the outline rasterizer.
# define USE_OUTLINE_RASTERIZER 0
#else
// in this case, we can optionally use the outline rasterizer (faster).
// NOTE: The outline rasterizer is different from the "general purpose"
// rasterizer and can speed up the stroking of lines. It has some problems
// though, for example the butts of the lines are not anti-aliased. So we
// use the much more powerfull general purpose rasterizer, and live with the
// performance hit for now. See _StrokePath().
// NOTE: The outline rasterizer will still be used for lines with 1 pixel width!
# define USE_OUTLINE_RASTERIZER 0
#endif


#define CHECK_CLIPPING	if (!fValidClipping) return BRect(0, 0, -1, -1);


// constructor
Painter::Painter()
	: fBuffer(NULL),
	  fPixelFormat(NULL),
	  fBaseRenderer(NULL),
	  fOutlineRenderer(NULL),
	  fOutlineRasterizer(NULL),
	  fUnpackedScanline(NULL),
	  fPackedScanline(NULL),
	  fRasterizer(NULL),
	  fRenderer(NULL),
	  fRendererBin(NULL),
	  fLineProfile(),
	  fSubpixelPrecise(false),

	  fPenSize(1.0),
	  fClippingRegion(new BRegion()),
	  fValidClipping(false),
	  fDrawingMode(B_OP_COPY),
	  fDrawingText(false),
	  fAlphaSrcMode(B_PIXEL_ALPHA),
	  fAlphaFncMode(B_ALPHA_OVERLAY),
	  fPenLocation(0.0, 0.0),
	  fLineCapMode(B_BUTT_CAP),
	  fLineJoinMode(B_MITER_JOIN),
	  fMiterLimit(B_DEFAULT_MITER_LIMIT),

	  fPatternHandler(new PatternHandler()),
	  fTextRenderer(new AGGTextRenderer())
{
	// Usually, the drawing engine will lock the font for us when
	// needed - unfortunately, it can't know we need it here
	fFont.Lock();
	_UpdateFont();
	fFont.Unlock();

#if USE_OUTLINE_RASTERIZER
	_UpdateLineWidth();
#endif
}

// destructor
Painter::~Painter()
{
	_MakeEmpty();

	delete fClippingRegion;
	delete fPatternHandler;
	delete fTextRenderer;
}

// #pragma mark -

// AttachToBuffer
void
Painter::AttachToBuffer(RenderingBuffer* buffer)
{
	if (buffer && buffer->InitCheck() >= B_OK &&
		// TODO: implement drawing on B_RGB24, B_RGB15, B_RGB16, B_CMAP8 and B_GRAY8 :-[
		(buffer->ColorSpace() == B_RGBA32 || buffer->ColorSpace() == B_RGB32)) {
		// clean up previous stuff
		_MakeEmpty();

		fBuffer = new agg::rendering_buffer();
		fBuffer->attach((uint8*)buffer->Bits(),
						buffer->Width(),
						buffer->Height(),
						buffer->BytesPerRow());

		fPixelFormat = new pixfmt(*fBuffer, fPatternHandler);
		fPixelFormat->SetDrawingMode(fDrawingMode, fAlphaSrcMode, fAlphaFncMode, false);

		fBaseRenderer = new renderer_base(*fPixelFormat);
		// attach our clipping region to the renderer, it keeps a pointer
		fBaseRenderer->set_clipping_region(fClippingRegion);

		// These are the AGG renderes and rasterizes which
		// will be used for stroking paths
#if USE_OUTLINE_RASTERIZER
#if ALIASED_DRAWING
		fOutlineRenderer = new outline_renderer_type(*fBaseRenderer);
		fOutlineRasterizer = new outline_rasterizer_type(*fOutlineRenderer);
#else
		fOutlineRenderer = new outline_renderer_type(*fBaseRenderer, fLineProfile);
		fOutlineRasterizer = new outline_rasterizer_type(*fOutlineRenderer);

		// attach our line profile to the renderer, it keeps a pointer
		fOutlineRenderer->profile(fLineProfile);
#endif // ALIASED_DRAWING
#endif // USE_OUTLINE_RASTERIZER
		// the renderer used for filling paths
		fRenderer = new renderer_type(*fBaseRenderer);
		fRasterizer = new rasterizer_type();
		fUnpackedScanline = new scanline_unpacked_type();
		fPackedScanline = new scanline_packed_type();

#if ALIASED_DRAWING
		fRasterizer->gamma(agg::gamma_threshold(0.5));
#endif

		// possibly needed for drawing text (if the font is
		// a one bit bitmap font, which is currently not supported yet)
		fRendererBin = new renderer_bin_type(*fBaseRenderer);

		_SetRendererColor(fPatternHandler->HighColor().GetColor32());
	}
}

// DetachFromBuffer
void
Painter::DetachFromBuffer()
{
	_MakeEmpty();
}

// #pragma mark -

// SetDrawState
void
Painter::SetDrawState(const DrawState* data, bool updateFont)
{
	// NOTE: The custom clipping in "data" is ignored, because it has already been
	// taken into account elsewhere

	// TODO: optimize "context switch" for speed...
	// but for now...
	SetPenSize(data->PenSize());
	SetPenLocation(data->PenLocation());

	if (updateFont)
		SetFont(data->Font());

	fTextRenderer->SetAntialiasing(!(data->ForceFontAliasing() || data->Font().Flags() & B_DISABLE_ANTIALIASING));

	fSubpixelPrecise = data->SubPixelPrecise();

	// any of these conditions means we need to use a different drawing
	// mode instance
	bool updateDrawingMode = !(data->GetPattern() == fPatternHandler->GetPattern()) ||
		data->GetDrawingMode() != fDrawingMode ||
		(data->GetDrawingMode() == B_OP_ALPHA && (data->AlphaSrcMode() != fAlphaSrcMode ||
												  data->AlphaFncMode() != fAlphaFncMode));

	fDrawingMode	= data->GetDrawingMode();
	fAlphaSrcMode	= data->AlphaSrcMode();
	fAlphaFncMode	= data->AlphaFncMode();
	fPatternHandler->SetPattern(data->GetPattern());
	fLineCapMode	= data->LineCapMode();
	fLineJoinMode	= data->LineJoinMode();
	fMiterLimit		= data->MiterLimit();

	// adopt the color *after* the pattern is set
	// to set the renderers to the correct color
	SetHighColor(data->HighColor().GetColor32());
	SetLowColor(data->LowColor().GetColor32());

	if (updateDrawingMode || fPixelFormat->UsesOpCopyForText())
		_UpdateDrawingMode();
}

// #pragma mark - state

// ConstrainClipping
void
Painter::ConstrainClipping(const BRegion* region)
{
	*fClippingRegion = *region;
	fValidClipping = fClippingRegion->Frame().IsValid();
// TODO: would be nice if we didn't need to copy a region
// for *each* drawing command...
//fBaseRenderer->set_clipping_region(const_cast<BRegion*>(region));
//fValidClipping = region->Frame().IsValid();
}

// SetHighColor
void
Painter::SetHighColor(const rgb_color& color)
{
	fPatternHandler->SetHighColor(color);
	if (*(fPatternHandler->GetR5Pattern()) == B_SOLID_HIGH)
		_SetRendererColor(color);
}

// SetLowColor
void
Painter::SetLowColor(const rgb_color& color)
{
	fPatternHandler->SetLowColor(color);
	if (*(fPatternHandler->GetR5Pattern()) == B_SOLID_LOW)
		_SetRendererColor(color);
}

// SetPenSize
void
Painter::SetPenSize(float size)
{
	if (fPenSize != size) {
		fPenSize = size;
#if USE_OUTLINE_RASTERIZER
// NOTE: _UpdateLineWidth() updates the line profile which is quite a heavy resource!
// fortunately, we don't need it when using the general purpose rasterizer
		_UpdateLineWidth();
#endif
	}
}

// SetPattern
void
Painter::SetPattern(const pattern& p, bool drawingText)
{
	if (!(p == *fPatternHandler->GetR5Pattern()) || drawingText != fDrawingText) {
		fPatternHandler->SetPattern(p);
		fDrawingText = drawingText;
		_UpdateDrawingMode(fDrawingText);

		// update renderer color if necessary
		if (fPatternHandler->IsSolidHigh()) {
			// pattern was not solid high before
			_SetRendererColor(fPatternHandler->HighColor().GetColor32());
		} else if (fPatternHandler->IsSolidLow()) {
			// pattern was not solid low before
			_SetRendererColor(fPatternHandler->LowColor().GetColor32());
		}
	}
}

// SetPenLocation
void
Painter::SetPenLocation(const BPoint& location)
{
	fPenLocation = location;
}

// SetFont
void
Painter::SetFont(const ServerFont& font)
{
	fFont = font;
	_UpdateFont();
}

// #pragma mark - drawing

// StrokeLine
BRect
Painter::StrokeLine(BPoint a, BPoint b)
{
	CHECK_CLIPPING

	// "false" means not to do the pixel center offset,
	// because it would mess up our optimized versions
	_Transform(&a, false);
	_Transform(&b, false);

	BRect touched(min_c(a.x, b.x), min_c(a.y, b.y),
				  max_c(a.x, b.x), max_c(a.y, b.y));

	// This is supposed to stop right here if we can see
	// that we're definitaly outside the clipping reagion.
	// Extending by penSize like that is not really correct, 
	// but fast and only triggers unnecessary calculation
	// in a few edge cases
	touched.InsetBy(-(fPenSize - 1), -(fPenSize - 1));
	if (!touched.Intersects(fClippingRegion->Frame())) {
		touched.Set(0.0, 0.0, -1.0, -1.0);
		return touched;
	}

	// first, try an optimized version
	if (fPenSize == 1.0 &&
		(fDrawingMode == B_OP_COPY || fDrawingMode == B_OP_OVER)) {
		pattern pat = *fPatternHandler->GetR5Pattern();
		if (pat == B_SOLID_HIGH &&
			StraightLine(a, b, fPatternHandler->HighColor().GetColor32())) {
			return _Clipped(touched);
		} else if (pat == B_SOLID_LOW &&
			StraightLine(a, b, fPatternHandler->LowColor().GetColor32())) {
			return _Clipped(touched);
		}
	}
	// do the pixel center offset here
	a.x += 0.5;
	a.y += 0.5;
	b.x += 0.5;
	b.y += 0.5;

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, b.y);

	touched = _StrokePath(path);

	return _Clipped(touched);
}

// StraightLine
bool
Painter::StraightLine(BPoint a, BPoint b, const rgb_color& c) const
{
	if (fBuffer && fValidClipping) {
		if (a.x == b.x) {
			// vertical
			uint8* dst = fBuffer->row(0);
			uint32 bpr = fBuffer->stride();
			int32 x = (int32)a.x;
			dst += x * 4;
			int32 y1 = (int32)min_c(a.y, b.y);
			int32 y2 = (int32)max_c(a.y, b.y);
			pixel32 color;
			color.data8[0] = c.blue;
			color.data8[1] = c.green;
			color.data8[2] = c.red;
			color.data8[3] = 255;
			// draw a line, iterate over clipping boxes
			fBaseRenderer->first_clip_box();
			do {
				if (fBaseRenderer->xmin() <= x &&
					fBaseRenderer->xmax() >= x) {
					int32 i = max_c(fBaseRenderer->ymin(), y1);
					int32 end = min_c(fBaseRenderer->ymax(), y2);
					uint8* handle = dst + i * bpr;
					for (; i <= end; i++) {
						*(uint32*)handle = color.data32;
						handle += bpr;
					}
				}
			} while (fBaseRenderer->next_clip_box());
	
			return true;
	
		} else if (a.y == b.y) {
			// horizontal
			int32 y = (int32)a.y;
			uint8* dst = fBuffer->row(y);
			int32 x1 = (int32)min_c(a.x, b.x);
			int32 x2 = (int32)max_c(a.x, b.x);
			pixel32 color;
			color.data8[0] = c.blue;
			color.data8[1] = c.green;
			color.data8[2] = c.red;
			color.data8[3] = 255;
			// draw a line, iterate over clipping boxes
			fBaseRenderer->first_clip_box();
			do {
				if (fBaseRenderer->ymin() <= y &&
					fBaseRenderer->ymax() >= y) {
					int32 i = max_c(fBaseRenderer->xmin(), x1);
					int32 end = min_c(fBaseRenderer->xmax(), x2);
					uint32* handle = (uint32*)(dst + i * 4);
					for (; i <= end; i++) {
						*handle++ = color.data32;
					}
				}
			} while (fBaseRenderer->next_clip_box());
	
			return true;
		}
	}
	return false;
}

// #pragma mark -

// StrokeTriangle
BRect
Painter::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3) const
{
	return _DrawTriangle(pt1, pt2, pt3, false);
}

// FillTriangle
BRect
Painter::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3) const
{
	return _DrawTriangle(pt1, pt2, pt3, true);
}

// DrawPolygon
BRect
Painter::DrawPolygon(BPoint* p, int32 numPts,
					 bool filled, bool closed) const
{
	CHECK_CLIPPING

	if (numPts > 0) {

		agg::path_storage path;
		_Transform(p);
		path.move_to(p->x, p->y);

		for (int32 i = 1; i < numPts; i++) {
			p++;
			_Transform(p);
			path.line_to(p->x, p->y);
		}

		if (closed)
			path.close_polygon();

		if (filled)
			return _FillPath(path);
		else
			return _StrokePath(path);
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}

// DrawBezier
BRect
Painter::DrawBezier(BPoint* p, bool filled) const
{
	CHECK_CLIPPING

	agg::path_storage curve;

	_Transform(&(p[0]));
	_Transform(&(p[1]));
	_Transform(&(p[2]));
	_Transform(&(p[3]));

	curve.move_to(p[0].x, p[0].y);
	curve.curve4(p[1].x, p[1].y,
				 p[2].x, p[2].y,
				 p[3].x, p[3].y);


	agg::conv_curve<agg::path_storage> path(curve);

	if (filled) {
		curve.close_polygon();
		return _FillPath(path);
	} else {
		return _StrokePath(path);
	}
}

// this comes from Shape.cpp
// code duplication ahead...
#define OP_LINETO		0x10000000
#define OP_BEZIERTO		0x20000000
#define OP_CLOSE		0x40000000
#define OP_MOVETO		0x80000000

// DrawShape
BRect
Painter::DrawShape(const int32& opCount, const uint32* opList,
				   const int32& ptCount, const BPoint* points,
				   bool filled) const
{
	CHECK_CLIPPING

	// TODO: if shapes are ever used more heavily in Haiku,
	// it would be nice to use BShape data directly (write
	// an AGG "VertexSource" adaptor)
	agg::path_storage path;
	for (int32 i = 0; i < opCount; i++) {
		uint32 op = opList[i] & 0xFF000000;
		if (op & OP_MOVETO) {
			path.move_to(points->x + fPenLocation.x, points->y + fPenLocation.y);
			points++;
		}

		if (op & OP_LINETO) {
			int32 count = opList[i] & 0x00FFFFFF;
			while (count--) {
				path.line_to(points->x + fPenLocation.x, points->y + fPenLocation.y);
				points++;
			}
		}

		if (op & OP_BEZIERTO) {
			int32 count = opList[i] & 0x00FFFFFF;
			while (count) {
				path.curve4(points[0].x + fPenLocation.x, points[0].y + fPenLocation.y,
							points[1].x + fPenLocation.x, points[1].y + fPenLocation.y,
							points[2].x + fPenLocation.x, points[2].y + fPenLocation.y);
				points += 3;
				count -= 3;
			}
		}

		if (op & OP_CLOSE)
			path.close_polygon();
	}

	agg::conv_curve<agg::path_storage> curve(path);
	if (filled)
		return _FillPath(curve);
	else
		return _StrokePath(curve);
}

// StrokeRect
BRect
Painter::StrokeRect(const BRect& r) const
{
	CHECK_CLIPPING

	// support invalid rects
	BPoint a(min_c(r.left, r.right), min_c(r.top, r.bottom));
	BPoint b(max_c(r.left, r.right), max_c(r.top, r.bottom));
	_Transform(&a, false);
	_Transform(&b, false);

	// first, try an optimized version
	if (fPenSize == 1.0 &&
		(fDrawingMode == B_OP_COPY || fDrawingMode == B_OP_OVER)) {
		pattern p = *fPatternHandler->GetR5Pattern();
		if (p == B_SOLID_HIGH) {
			BRect rect(a, b);
			StrokeRect(rect,
					   fPatternHandler->HighColor().GetColor32());
			return _Clipped(rect);
		} else if (p == B_SOLID_LOW) {
			BRect rect(a, b);
			StrokeRect(rect,
					   fPatternHandler->LowColor().GetColor32());
			return _Clipped(rect);
		}
	}

	if (fmodf(fPenSize, 2.0) != 0.0) {
		// shift coords to center of pixels
		a.x += 0.5;
		a.y += 0.5;
		b.x += 0.5;
		b.y += 0.5;
	}

	agg::path_storage path;
	path.move_to(a.x, a.y);
	if (a.x == b.x || a.y == b.y) {
		// special case rects with one pixel height or width
		path.line_to(b.x, b.y);
	} else {
		path.line_to(b.x, a.y);
		path.line_to(b.x, b.y);
		path.line_to(a.x, b.y);
	}
	path.close_polygon();

	return _StrokePath(path);
}

// StrokeRect
void
Painter::StrokeRect(const BRect& r, const rgb_color& c) const
{
	StraightLine(BPoint(r.left, r.top),
				 BPoint(r.right - 1, r.top), c);
	StraightLine(BPoint(r.right, r.top),
				 BPoint(r.right, r.bottom - 1), c);
	StraightLine(BPoint(r.right, r.bottom),
				 BPoint(r.left + 1, r.bottom), c);
	StraightLine(BPoint(r.left, r.bottom),
				 BPoint(r.left, r.top + 1), c);
}

// FillRect
BRect
Painter::FillRect(const BRect& r) const
{
	CHECK_CLIPPING

	// support invalid rects
	BPoint a(min_c(r.left, r.right), min_c(r.top, r.bottom));
	BPoint b(max_c(r.left, r.right), max_c(r.top, r.bottom));
	_Transform(&a, false);
	_Transform(&b, false);

	// first, try an optimized version
	if (fDrawingMode == B_OP_COPY || fDrawingMode == B_OP_OVER) {
		pattern p = *fPatternHandler->GetR5Pattern();
		if (p == B_SOLID_HIGH) {
			BRect rect(a, b);
			FillRect(rect, fPatternHandler->HighColor().GetColor32());
			return _Clipped(rect);
		} else if (p == B_SOLID_LOW) {
			BRect rect(a, b);
			FillRect(rect, fPatternHandler->LowColor().GetColor32());
			return _Clipped(rect);
		}
	}
	if (fDrawingMode == B_OP_ALPHA && fAlphaFncMode == B_ALPHA_OVERLAY) {
		pattern p = *fPatternHandler->GetR5Pattern();
		if (p == B_SOLID_HIGH) {
			BRect rect(a, b);
			_BlendRect32(rect, fPatternHandler->HighColor().GetColor32());
			return _Clipped(rect);
		} else if (p == B_SOLID_LOW) {
			rgb_color c = fPatternHandler->LowColor().GetColor32();
			if (fAlphaSrcMode == B_CONSTANT_ALPHA)
				c.alpha = fPatternHandler->HighColor().GetColor32().alpha;
			BRect rect(a, b);
			_BlendRect32(rect, c);
			return _Clipped(rect);
		}
	}
	

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	b.x += 1.0;
	b.y += 1.0;

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, a.y);
	path.line_to(b.x, b.y);
	path.line_to(a.x, b.y);
	path.close_polygon();

	return _FillPath(path);
}

// FillRect
void
Painter::FillRect(const BRect& r, const rgb_color& c) const
{
	if (fBuffer && fValidClipping) {
		uint8* dst = fBuffer->row(0);
		uint32 bpr = fBuffer->stride();
		int32 left = (int32)r.left;
		int32 top = (int32)r.top;
		int32 right = (int32)r.right;
		int32 bottom = (int32)r.bottom;
		// get a 32 bit pixel ready with the color
		pixel32 color;
		color.data8[0] = c.blue;
		color.data8[1] = c.green;
		color.data8[2] = c.red;
		color.data8[3] = c.alpha;
		// fill rects, iterate over clipping boxes
		fBaseRenderer->first_clip_box();
		do {
			int32 x1 = max_c(fBaseRenderer->xmin(), left);
			int32 x2 = min_c(fBaseRenderer->xmax(), right);
			if (x1 <= x2) {
				int32 y1 = max_c(fBaseRenderer->ymin(), top);
				int32 y2 = min_c(fBaseRenderer->ymax(), bottom);
				uint8* offset = dst + x1 * 4;
				for (; y1 <= y2; y1++) {
//					uint32* handle = (uint32*)(offset + y1 * bpr);
//					for (int32 x = x1; x <= x2; x++) {
//						*handle++ = color.data32;
//					}
gfxset32(offset + y1 * bpr, color.data32, (x2 - x1 + 1) * 4);
				}
			}
		} while (fBaseRenderer->next_clip_box());
	}
}

// FillRectNoClipping
void
Painter::FillRectNoClipping(const BRect& r, const rgb_color& c) const
{
	if (fBuffer) {
		int32 left = (int32)r.left;
		int32 y = (int32)r.top;
		int32 right = (int32)r.right;
		int32 bottom = (int32)r.bottom;

		uint8* dst = fBuffer->row(y);
		uint32 bpr = fBuffer->stride();

		// get a 32 bit pixel ready with the color
		pixel32 color;
		color.data8[0] = c.blue;
		color.data8[1] = c.green;
		color.data8[2] = c.red;
		color.data8[3] = c.alpha;

		dst += left * 4;

		for (; y <= bottom; y++) {
//			uint32* handle = (uint32*)dst;
//			for (int32 x = left; x <= right; x++) {
//				*handle++ = color.data32;
//			}
gfxset32(dst, color.data32, (right - left + 1) * 4);
			dst += bpr;
		}
	}
}

// StrokeRoundRect
BRect
Painter::StrokeRoundRect(const BRect& r, float xRadius, float yRadius) const
{
	CHECK_CLIPPING

	BPoint lt(r.left, r.top);
	BPoint rb(r.right, r.bottom);
	bool centerOffset = fPenSize == 1.0;
	// TODO: use this when using _StrokePath()
	// bool centerOffset = fmodf(fPenSize, 2.0) != 0.0;
	_Transform(&lt, centerOffset);
	_Transform(&rb, centerOffset);

	if (fPenSize == 1.0) {
		agg::rounded_rect rect;
		rect.rect(lt.x, lt.y, rb.x, rb.y);
		rect.radius(xRadius, yRadius);
	
		return _StrokePath(rect);
	} else {
		// NOTE: This implementation might seem a little strange, but it makes
		// stroked round rects look like on R5. A more correct way would be to use
		// _StrokePath() as above (independent from fPenSize).
		// The fact that the bounding box of the round rect is not enlarged
		// by fPenSize/2 is actually on purpose, though one could argue it is unexpected.

		// enclose the right and bottom edge	
		rb.x++;
		rb.y++;
	
		agg::rounded_rect outer;
		outer.rect(lt.x, lt.y, rb.x, rb.y);
		outer.radius(xRadius, yRadius);

		fRasterizer->reset();
		fRasterizer->add_path(outer);

		// don't add an inner hole if the "size is negative", this avoids some
		// defects that can be observed on R5 and could be regarded as a bug.
		if (2 * fPenSize < rb.x - lt.x && 2 * fPenSize < rb.y - lt.y) {
			agg::rounded_rect inner;
			inner.rect(lt.x + fPenSize, lt.y + fPenSize, rb.x - fPenSize, rb.y - fPenSize);
			inner.radius(max_c(0.0, xRadius - fPenSize), max_c(0.0, yRadius - fPenSize));
		
			fRasterizer->add_path(inner);
		}
	
		// make the inner rect work as a hole
		fRasterizer->filling_rule(agg::fill_even_odd);

		if (fPenSize > 4)
			agg::render_scanlines(*fRasterizer, *fPackedScanline, *fRenderer);
		else
			agg::render_scanlines(*fRasterizer, *fUnpackedScanline, *fRenderer);
	
		// reset to default
		fRasterizer->filling_rule(agg::fill_non_zero);
	
		return _Clipped(_BoundingBox(outer));
	}
}

// FillRoundRect
BRect
Painter::FillRoundRect(const BRect& r, float xRadius, float yRadius) const
{
	CHECK_CLIPPING

	BPoint lt(r.left, r.top);
	BPoint rb(r.right, r.bottom);
	_Transform(&lt, false);
	_Transform(&rb, false);

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	rb.x += 1.0;
	rb.y += 1.0;

	agg::rounded_rect rect;
	rect.rect(lt.x, lt.y, rb.x, rb.y);
	rect.radius(xRadius, yRadius);

	return _FillPath(rect);
}
									
// DrawEllipse
BRect
Painter::DrawEllipse(BRect r, bool fill) const
{
	CHECK_CLIPPING

	if (!fSubpixelPrecise) {
		// align rect to pixels
		align_rect_to_pixels(&r);
		// account for "pixel index" versus "pixel area"
		r.right++;
		r.bottom++;
		if (!fill && fmodf(fPenSize, 2.0) != 0.0) {
			// align the stroke
			r.InsetBy(0.5, 0.5);
		}
	}

	float xRadius = r.Width() / 2.0;
	float yRadius = r.Height() / 2.0;
	BPoint center(r.left + xRadius, r.top + yRadius);

	int32 divisions = (int32)((xRadius + yRadius + 2 * fPenSize) * PI / 2);
	if (divisions < 12)
		divisions = 12;
	// TODO: arbitrarily set upper limit - should be tested...
	if (divisions > 32768)
		divisions = 32768;

	if (fill) {
		agg::ellipse path(center.x, center.y, xRadius, yRadius, divisions);

		return _FillPath(path);
	} else {
		// NOTE: This implementation might seem a little strange, but it makes
		// stroked ellipses look like on R5. A more correct way would be to use
		// _StrokePath(), but it currently has its own set of problems with narrow
		// ellipses (for small xRadii or yRadii).
		float inset = fPenSize / 2.0;
		agg::ellipse inner(center.x, center.y,
						   max_c(0.0, xRadius - inset),
						   max_c(0.0, yRadius - inset),
						   divisions);
		agg::ellipse outer(center.x, center.y,
						   xRadius + inset,
						   yRadius + inset,
						   divisions);

		fRasterizer->reset();
		fRasterizer->add_path(outer);
		fRasterizer->add_path(inner);

		// make the inner ellipse work as a hole
		fRasterizer->filling_rule(agg::fill_even_odd);

		if (fPenSize > 4)
			agg::render_scanlines(*fRasterizer, *fPackedScanline, *fRenderer);
		else
			agg::render_scanlines(*fRasterizer, *fUnpackedScanline, *fRenderer);

		// reset to default
		fRasterizer->filling_rule(agg::fill_non_zero);

		return _Clipped(_BoundingBox(outer));
	}
}

// StrokeArc
BRect
Painter::StrokeArc(BPoint center, float xRadius, float yRadius,
				   float angle, float span) const
{
	CHECK_CLIPPING

	_Transform(&center);

	double angleRad = (angle * PI) / 180.0;
	double spanRad = (span * PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius,
						-angleRad, -spanRad);

	agg::conv_curve<agg::bezier_arc> path(arc);
	path.approximation_scale(2.0);

	return _StrokePath(path);
}

// FillArc
BRect
Painter::FillArc(BPoint center, float xRadius, float yRadius,
				 float angle, float span) const
{
	CHECK_CLIPPING

	_Transform(&center);

	double angleRad = (angle * PI) / 180.0;
	double spanRad = (span * PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius,
						-angleRad, -spanRad);

	agg::conv_curve<agg::bezier_arc> segmentedArc(arc);

	agg::path_storage path;

	// build a new path by starting at the center point,
	// then traversing the arc, then going back to the center
	path.move_to(center.x, center.y);

	segmentedArc.rewind(0);
	double x;
	double y;
	unsigned cmd = segmentedArc.vertex(&x, &y);
	while (!agg::is_stop(cmd)) {
		path.line_to(x, y);
		cmd = segmentedArc.vertex(&x, &y);
	}

	path.close_polygon();

	return _FillPath(path);
}

// #pragma mark -

// DrawString
BRect
Painter::DrawString(const char* utf8String, uint32 length,
					BPoint baseLine, const escapement_delta* delta)
{
	CHECK_CLIPPING

	if (!fSubpixelPrecise) {
		baseLine.x = roundf(baseLine.x);
		baseLine.y = roundf(baseLine.y);
	}

	BRect bounds(0.0, 0.0, -1.0, -1.0);

	SetPattern(B_SOLID_HIGH, true);

	if (fBuffer) {
		bounds = fTextRenderer->RenderString(utf8String,
											 length,
											 fRenderer,
											 fRendererBin,
											 baseLine,
											 fClippingRegion->Frame(),
											 false,
											 &fPenLocation, 
											 delta);
	}
	return _Clipped(bounds);
}

// BoundingBox
BRect
Painter::BoundingBox(const char* utf8String, uint32 length,
					 BPoint baseLine, BPoint* penLocation,
					 const escapement_delta* delta) const
{
	if (!fSubpixelPrecise) {
		baseLine.x = roundf(baseLine.x);
		baseLine.y = roundf(baseLine.y);
	}

	static BRect dummy;
	return fTextRenderer->RenderString(utf8String,
									   length,
									   fRenderer,
									   fRendererBin,
									   baseLine, dummy, true, penLocation,
									   delta);
}

// StringWidth
float
Painter::StringWidth(const char* utf8String, uint32 length, const DrawState* context)
{
	SetFont(context->Font());
	return fTextRenderer->StringWidth(utf8String, length);
}

// #pragma mark -

// DrawBitmap
BRect
Painter::DrawBitmap(const ServerBitmap* bitmap,
					BRect bitmapRect, BRect viewRect) const
{
	CHECK_CLIPPING

	BRect touched = _Clipped(viewRect);

	if (bitmap && bitmap->IsValid() && touched.IsValid()) {
		// the native bitmap coordinate system
		BRect actualBitmapRect(bitmap->Bounds());

		agg::rendering_buffer srcBuffer;
		srcBuffer.attach(bitmap->Bits(),
						 bitmap->Width(),
						 bitmap->Height(),
						 bitmap->BytesPerRow());

		_DrawBitmap(srcBuffer, bitmap->ColorSpace(), actualBitmapRect, bitmapRect, viewRect);
	}
	return touched;
}

// #pragma mark -

// FillRegion
BRect
Painter::FillRegion(const BRegion* region) const
{
	CHECK_CLIPPING

	BRegion copy(*region);
	int32 count = copy.CountRects();
	BRect touched = FillRect(copy.RectAt(0));
	for (int32 i = 1; i < count; i++) {
		touched = touched | FillRect(copy.RectAt(i));
	}
	return touched;
}

// InvertRect
BRect
Painter::InvertRect(const BRect& r) const
{
	CHECK_CLIPPING

	BRegion region(r);
	if (fClippingRegion) {
		region.IntersectWith(fClippingRegion);
	}
	// implementation only for B_RGB32 at the moment
	int32 count = region.CountRects();
	for (int32 i = 0; i < count; i++) {
		_InvertRect32(region.RectAt(i));
	}
	return _Clipped(r);
}

// #pragma mark - private

// _MakeEmpty
void
Painter::_MakeEmpty()
{
	delete fBuffer;
	fBuffer = NULL;

	delete fPixelFormat;
	fPixelFormat = NULL;

	delete fBaseRenderer;
	fBaseRenderer = NULL;

#if USE_OUTLINE_RASTERIZER
	delete fOutlineRenderer;
	fOutlineRenderer = NULL;

	delete fOutlineRasterizer;
	fOutlineRasterizer = NULL;
#endif

	delete fUnpackedScanline;
	fUnpackedScanline = NULL;
	delete fPackedScanline;
	fPackedScanline = NULL;

	delete fRasterizer;
	fRasterizer = NULL;

	delete fRenderer;
	fRenderer = NULL;

	delete fRendererBin;
	fRendererBin = NULL;
}

// _Transform
void
Painter::_Transform(BPoint* point, bool centerOffset) const
{
	// rounding
	if (!fSubpixelPrecise) {
		// TODO: validate usage of floor() for values < 0
		point->x = floorf(point->x);
		point->y = floorf(point->y);
	}
	// this code is supposed to move coordinates to the center of pixels,
	// as AGG considers (0,0) to be the "upper left corner" of a pixel,
	// but BViews are less strict on those details
	if (centerOffset) {
		point->x += 0.5;
		point->y += 0.5;
	}
}

// _Transform
BPoint
Painter::_Transform(const BPoint& point, bool centerOffset) const
{
	BPoint ret = point;
	_Transform(&ret, centerOffset);
	return ret;
}

// _Clipped
BRect
Painter::_Clipped(const BRect& rect) const
{
	if (rect.IsValid()) {
		return BRect(rect & fClippingRegion->Frame());
	}
	return BRect(rect);
}

// _UpdateFont
void
Painter::_UpdateFont()
{
	fTextRenderer->SetFont(fFont);
}

// _UpdateLineWidth
void
Painter::_UpdateLineWidth()
{
	fLineProfile.width(fPenSize);	
}

// _UpdateDrawingMode
void
Painter::_UpdateDrawingMode(bool drawingText)
{
	// The AGG renderers have their own color setting, however
	// almost all drawing mode classes ignore the color given
	// by the AGG renderer and use the colors from the PatternHandler
	// instead. If we have a B_SOLID_* pattern, we can actually use
	// the color in the renderer and special versions of drawing modes
	// that don't use PatternHandler and are more efficient. This
	// has been implemented for B_OP_COPY and a couple others (the
	// DrawingMode*Solid ones) as of now. The PixelFormat knows the
	// PatternHandler and makes its decision based on the pattern.
	// The last parameter to SetDrawingMode() is a flag if a special
	// for when Painter is used to draw text. In this case, another
	// special version of B_OP_COPY is used that acts like R5 in that
	// anti-aliased pixel are not rendered against the actual background
	// but the current low color instead. This way, the frame buffer
	// doesn't need to be read.
	// When a solid pattern is used, _SetRendererColor()
	// has to be called so that all internal colors in the renderes
	// are up to date for use by the solid drawing mode version.
	fPixelFormat->SetDrawingMode(fDrawingMode, fAlphaSrcMode,
								 fAlphaFncMode, drawingText);
}

// _SetRendererColor
void
Painter::_SetRendererColor(const rgb_color& color) const
{
#if USE_OUTLINE_RASTERIZER
	if (fOutlineRenderer)
#if ALIASED_DRAWING
		fOutlineRenderer->line_color(agg::rgba(color.red / 255.0,
											   color.green / 255.0,
											   color.blue / 255.0,
											   color.alpha / 255.0));
#else
		fOutlineRenderer->color(agg::rgba(color.red / 255.0,
										  color.green / 255.0,
										  color.blue / 255.0,
										  color.alpha / 255.0));
#endif // ALIASED_DRAWING
#endif // USE_OUTLINE_RASTERIZER
	if (fRenderer)
		fRenderer->color(agg::rgba(color.red / 255.0,
								   color.green / 255.0,
								   color.blue / 255.0,
								   color.alpha / 255.0));
// TODO: bitmap fonts not yet correctly setup in AGGTextRenderer
//	if (fRendererBin)
//		fRendererBin->color(agg::rgba(color.red / 255.0,
//									  color.green / 255.0,
//									  color.blue / 255.0,
//									  color.alpha / 255.0));

}

// #pragma mark -

// _DrawTriangle
inline BRect
Painter::_DrawTriangle(BPoint pt1, BPoint pt2, BPoint pt3, bool fill) const
{
	CHECK_CLIPPING

	_Transform(&pt1);
	_Transform(&pt2);
	_Transform(&pt3);

	agg::path_storage path;

	path.move_to(pt1.x, pt1.y);
	path.line_to(pt2.x, pt2.y);
	path.line_to(pt3.x, pt3.y);

	path.close_polygon();

	if (fill)
		return _FillPath(path);
	else
		return _StrokePath(path);
}

// copy_bitmap_row_cmap8_copy
static inline void
copy_bitmap_row_cmap8_copy(uint8* dst, const uint8* src, int32 numPixels,
						   const rgb_color* colorMap)
{
	uint32* d = (uint32*)dst;
	const uint8* s = src;
	while (numPixels--) {
		const rgb_color c = colorMap[*s++];
		*d++ = (c.alpha << 24) | (c.red << 16) | (c.green << 8) | (c.blue);
	}
}

// copy_bitmap_row_cmap8_over
static inline void
copy_bitmap_row_cmap8_over(uint8* dst, const uint8* src, int32 numPixels,
						   const rgb_color* colorMap)
{
	uint32* d = (uint32*)dst;
	const uint8* s = src;
	while (numPixels--) {
		const rgb_color c = colorMap[*s++];
		if (c.alpha)
			*d = (c.alpha << 24) | (c.red << 16) | (c.green << 8) | (c.blue);
		d++;
	}
}

// copy_bitmap_row_bgr32_copy
static inline void
copy_bitmap_row_bgr32_copy(uint8* dst, const uint8* src, int32 numPixels,
						   const rgb_color* colorMap)
{
	memcpy(dst, src, numPixels * 4);
}

// copy_bitmap_row_bgr32_alpha
static inline void
copy_bitmap_row_bgr32_alpha(uint8* dst, const uint8* src, int32 numPixels,
							const rgb_color* colorMap)
{
	uint32* d = (uint32*)dst;
	int32 bytes = numPixels * 4;
	uint8 buffer[bytes];
	uint8* b = buffer;
	while (numPixels--) {
		if (src[3] == 255) {
			*(uint32*)b = *(uint32*)src;
		} else {
			*(uint32*)b = *d;
			b[0] = ((src[0] - b[0]) * src[3] + (b[0] << 8)) >> 8;
			b[1] = ((src[1] - b[1]) * src[3] + (b[1] << 8)) >> 8;
			b[2] = ((src[2] - b[2]) * src[3] + (b[2] << 8)) >> 8;
		}
		d ++;
		b += 4;
		src += 4;
	}
	memcpy(dst, buffer, bytes);
}

// _DrawBitmap
void
Painter::_DrawBitmap(const agg::rendering_buffer& srcBuffer, color_space format,
					 BRect actualBitmapRect, BRect bitmapRect, BRect viewRect) const
{
	if (!fBuffer || !fValidClipping
		|| !bitmapRect.IsValid() || !bitmapRect.Intersects(actualBitmapRect)
		|| !viewRect.IsValid()) {
		return;
	}

	if (!fSubpixelPrecise)
		align_rect_to_pixels(&viewRect);

	double xScale = (viewRect.Width() + 1) / (bitmapRect.Width() + 1);
	double yScale = (viewRect.Height() + 1) / (bitmapRect.Height() + 1);

	if (xScale == 0.0 || yScale == 0.0)
		return;

	// compensate for the lefttop offset the actualBitmapRect might have
// NOTE: I have no clue why enabling the next call gives a wrong result!
// According to the BeBook, bitmapRect is supposed to be in native
// bitmap space! Disabling this call makes it look like the bitmap bounds are
// assumed to have a left/top coord of 0,0 at all times. This is simply not true.
//		bitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);
	// actualBitmapRect has the right size, but put it at B_ORIGIN too
	actualBitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);

	// constrain rect to passed bitmap bounds
	// and transfer the changes to the viewRect
	if (bitmapRect.left < actualBitmapRect.left) {
		float diff = actualBitmapRect.left - bitmapRect.left;
		viewRect.left += diff;
		bitmapRect.left = actualBitmapRect.left;
	}
	if (bitmapRect.top < actualBitmapRect.top) {
		float diff = actualBitmapRect.top - bitmapRect.top;
		viewRect.top += diff;
		bitmapRect.top = actualBitmapRect.top;
	}
	if (bitmapRect.right > actualBitmapRect.right) {
		float diff = bitmapRect.right - actualBitmapRect.right;
		viewRect.right -= diff;
		bitmapRect.right = actualBitmapRect.right;
	}
	if (bitmapRect.bottom > actualBitmapRect.bottom) {
		float diff = bitmapRect.right - actualBitmapRect.bottom;
		viewRect.bottom -= diff;
		bitmapRect.bottom = actualBitmapRect.bottom;
	}

	double xOffset = viewRect.left - bitmapRect.left;
	double yOffset = viewRect.top - bitmapRect.top;

	switch (format) {
		case B_RGB32:
		case B_RGBA32: {
			bool generic = true;
			// maybe we can use an optimized version
			if (xScale == 1.0 && yScale == 1.0) {
				if (fDrawingMode == B_OP_COPY) {
					_DrawBitmapNoScale32(copy_bitmap_row_bgr32_copy, 4, srcBuffer, xOffset, yOffset, viewRect);
					generic = false;
				} else if (fDrawingMode == B_OP_ALPHA
						 && fAlphaSrcMode == B_PIXEL_ALPHA && fAlphaFncMode == B_ALPHA_OVERLAY) {
					_DrawBitmapNoScale32(copy_bitmap_row_bgr32_alpha, 4, srcBuffer, xOffset, yOffset, viewRect);
					generic = false;
				}
			}

			if (generic) {
				_DrawBitmapGeneric32(srcBuffer, xOffset, yOffset, xScale, yScale, viewRect);
			}
			break;
		}
		default: {
			bool generic = true;
			if (format == B_CMAP8 && xScale == 1.0 && yScale == 1.0) {
				if (fDrawingMode == B_OP_COPY) {
					_DrawBitmapNoScale32(copy_bitmap_row_cmap8_copy, 1, srcBuffer, xOffset, yOffset, viewRect);
					generic = false;
				} else if (fDrawingMode == B_OP_OVER) {
					_DrawBitmapNoScale32(copy_bitmap_row_cmap8_over, 1, srcBuffer, xOffset, yOffset, viewRect);
					generic = false;
				}
			}

			if (generic) {
				// TODO: this is only a temporary implementation,
				// to really handle other colorspaces, one would
				// rather do the conversion with much less overhead,
				// for example in the nn filter (hm), or in the
				// scanline generator (better)
				// maybe we can use an optimized version
				BBitmap temp(actualBitmapRect, B_BITMAP_NO_SERVER_LINK, B_RGBA32);
				status_t err = temp.ImportBits(srcBuffer.buf(),
											   srcBuffer.height() * srcBuffer.stride(),
											   srcBuffer.stride(),
											   0, format);
				if (err >= B_OK) {
					agg::rendering_buffer convertedBuffer;
					convertedBuffer.attach((uint8*)temp.Bits(),
										   (uint32)actualBitmapRect.IntegerWidth() + 1,
										   (uint32)actualBitmapRect.IntegerHeight() + 1,
										   temp.BytesPerRow());
					_DrawBitmapGeneric32(convertedBuffer, xOffset, yOffset, xScale, yScale, viewRect);
				} else {
					fprintf(stderr, "Painter::_DrawBitmap() - colorspace conversion failed: %s\n", strerror(err));
				}
			}
			break;
		}
	}
}

// _DrawBitmapNoScale32
template <class F>
void
Painter::_DrawBitmapNoScale32(F copyRowFunction, uint32 bytesPerSourcePixel,
							  const agg::rendering_buffer& srcBuffer,
							  int32 xOffset, int32 yOffset, BRect viewRect) const
{
	// NOTE: this would crash if viewRect was large enough to read outside the
	// bitmap, so make sure this is not the case before calling this function!
	uint8* dst = fBuffer->row(0);
	uint32 dstBPR = fBuffer->stride();

	const uint8* src = srcBuffer.row(0);
	uint32 srcBPR = srcBuffer.stride();

	int32 left = (int32)viewRect.left;
	int32 top = (int32)viewRect.top;
	int32 right = (int32)viewRect.right;
	int32 bottom = (int32)viewRect.bottom;

	const rgb_color* colorMap = SystemPalette();

	// copy rects, iterate over clipping boxes
	fBaseRenderer->first_clip_box();
	do {
		int32 x1 = max_c(fBaseRenderer->xmin(), left);
		int32 x2 = min_c(fBaseRenderer->xmax(), right);
		if (x1 <= x2) {
			int32 y1 = max_c(fBaseRenderer->ymin(), top);
			int32 y2 = min_c(fBaseRenderer->ymax(), bottom);
			if (y1 <= y2) {
				uint8* dstHandle = dst + y1 * dstBPR + x1 * 4;
				const uint8* srcHandle = src + (y1 - yOffset) * srcBPR + (x1 - xOffset) * bytesPerSourcePixel;

				for (; y1 <= y2; y1++) {
					copyRowFunction(dstHandle, srcHandle, x2 - x1 + 1, colorMap);

					dstHandle += dstBPR;
					srcHandle += srcBPR;
				}
			}
		}
	} while (fBaseRenderer->next_clip_box());
}

// _DrawBitmapGeneric32
void
Painter::_DrawBitmapGeneric32(const agg::rendering_buffer& srcBuffer,
							  double xOffset, double yOffset,
							  double xScale, double yScale,
							  BRect viewRect) const
{
	typedef agg::span_allocator<agg::rgba8> span_alloc_type;
	
	// AGG pipeline
	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::span_image_filter_rgba32_nn<agg::order_bgra32,
											 interpolator_type> scaled_span_gen_type;
	typedef agg::renderer_scanline_aa<renderer_base, scaled_span_gen_type> scaled_image_renderer_type;


	agg::trans_affine srcMatrix;
//	srcMatrix *= agg::trans_affine_translation(-actualBitmapRect.left, -actualBitmapRect.top);

	agg::trans_affine imgMatrix;
	imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
	imgMatrix *= agg::trans_affine_translation(xOffset, yOffset);
	imgMatrix.invert();
	
	span_alloc_type sa;
	interpolator_type interpolator(imgMatrix);

	agg::rasterizer_scanline_aa<> pf;
	agg::scanline_u8 sl;

	// clip to the current clipping region's frame
	viewRect = viewRect & fClippingRegion->Frame();
	// convert to pixel coords (versus pixel indices)
	viewRect.right++;
	viewRect.bottom++;

	// path encloses image
	agg::path_storage path;
	path.move_to(viewRect.left, viewRect.top);
	path.line_to(viewRect.right, viewRect.top);
	path.line_to(viewRect.right, viewRect.bottom);
	path.line_to(viewRect.left, viewRect.bottom);
	path.close_polygon();

	agg::conv_transform<agg::path_storage> tr(path, srcMatrix);

	pf.add_path(tr);

	scaled_span_gen_type sg(sa, srcBuffer, agg::rgba(0, 0, 0, 0), interpolator);
	scaled_image_renderer_type ri(*fBaseRenderer, sg);

	agg::render_scanlines(pf, sl, ri);
}

// _InvertRect32
void
Painter::_InvertRect32(BRect r) const
{
	if (fBuffer) {
		int32 width = r.IntegerWidth() + 1;
		for (int32 y = (int32)r.top; y <= (int32)r.bottom; y++) {
			uint8* dst = fBuffer->row(y);
			dst += (int32)r.left * 4;
			for (int32 i = 0; i < width; i++) {
				dst[0] = 255 - dst[0];
				dst[1] = 255 - dst[1];
				dst[2] = 255 - dst[2];
				dst += 4;
			}
		}
	}
}

// _BlendRect32
void
Painter::_BlendRect32(const BRect& r, const rgb_color& c) const
{
	if (fBuffer && fValidClipping) {
		uint8* dst = fBuffer->row(0);
		uint32 bpr = fBuffer->stride();

		int32 left = (int32)r.left;
		int32 top = (int32)r.top;
		int32 right = (int32)r.right;
		int32 bottom = (int32)r.bottom;

		// fill rects, iterate over clipping boxes
		fBaseRenderer->first_clip_box();
		do {
			int32 x1 = max_c(fBaseRenderer->xmin(), left);
			int32 x2 = min_c(fBaseRenderer->xmax(), right);
			if (x1 <= x2) {
				int32 y1 = max_c(fBaseRenderer->ymin(), top);
				int32 y2 = min_c(fBaseRenderer->ymax(), bottom);

				uint8* offset = dst + x1 * 4 + y1 * bpr;
				for (; y1 <= y2; y1++) {
					blend_line32(offset, x2 - x1 + 1, c.red, c.green, c.blue, c.alpha);
					offset += bpr;
				}
			}
		} while (fBaseRenderer->next_clip_box());
	}
}

// #pragma mark -

template<class VertexSource>
BRect
Painter::_BoundingBox(VertexSource& path) const
{
	double left = 0.0;
	double top = 0.0;
	double right = -1.0;
	double bottom = -1.0;
	uint32 pathID[1];
	pathID[0] = 0;
	agg::bounding_rect(path, pathID, 0, 1, &left, &top, &right, &bottom);
	return BRect(left, top, right, bottom);
}

// agg_line_cap_mode_for
inline agg::line_cap_e
agg_line_cap_mode_for(cap_mode mode)
{
	switch (mode) {
		case B_BUTT_CAP:
			return agg::butt_cap;
		case B_SQUARE_CAP:
			return agg::square_cap;
		case B_ROUND_CAP:
			return agg::round_cap;
	}
	return agg::butt_cap;
}

// agg_line_join_mode_for
inline agg::line_join_e
agg_line_join_mode_for(join_mode mode)
{
	switch (mode) {
		case B_MITER_JOIN:
			return agg::miter_join;
		case B_ROUND_JOIN:
			return agg::round_join;
		case B_BEVEL_JOIN:
		case B_BUTT_JOIN: // ??
		case B_SQUARE_JOIN: // ??
			return agg::bevel_join;
	}
	return agg::miter_join;
}

// _StrokePath
template<class VertexSource>
BRect
Painter::_StrokePath(VertexSource& path) const
{
#if USE_OUTLINE_RASTERIZER
	fOutlineRasterizer->add_path(path);
#else
//	if (fPenSize > 1.0) {
		agg::conv_stroke<VertexSource> stroke(path);
		stroke.width(fPenSize);

		// special case line width = 1 with square caps
		// this has a couple of advantages and it looks
		// like this is also the R5 behaviour.
		if (fPenSize == 1.0 && fLineCapMode == B_BUTT_CAP) {
			stroke.line_cap(agg::square_cap);
		} else {
			stroke.line_cap(agg_line_cap_mode_for(fLineCapMode));
		}
		stroke.line_join(agg_line_join_mode_for(fLineJoinMode));
		stroke.miter_limit(fMiterLimit);

		fRasterizer->reset();
		agg::conv_clip_polygon<agg::conv_stroke<VertexSource> > clippedPath(stroke);
		clippedPath.clip_box(-500, -500, fBuffer->width() + 500, fBuffer->height() + 500);
		fRasterizer->add_path(clippedPath);

		if (fPenSize > 4)
			agg::render_scanlines(*fRasterizer, *fPackedScanline, *fRenderer);
		else
			agg::render_scanlines(*fRasterizer, *fUnpackedScanline, *fRenderer);
//	} else {
	// TODO: update to AGG 2.3 to get rid of the remaining problems:
	// rects which are 2 or 1 pixel high/wide don't render at all.
//		fOutlineRasterizer->add_path(path);
//	}
#endif

	BRect touched = _BoundingBox(path);
	float penSize = ceilf(fPenSize / 2.0);
	touched.InsetBy(-penSize, -penSize);

	return _Clipped(touched);
}

// _FillPath
template<class VertexSource>
BRect
Painter::_FillPath(VertexSource& path) const
{
	fRasterizer->reset();
	agg::conv_clip_polygon<VertexSource> clippedPath(path);
	clippedPath.clip_box(-500, -500, fBuffer->width() + 500, fBuffer->height() + 500);
	fRasterizer->add_path(clippedPath);
	agg::render_scanlines(*fRasterizer, *fPackedScanline, *fRenderer);

	return _Clipped(_BoundingBox(clippedPath));
}

