/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
 * rendering pipe-lines for stroke, fills, bitmap and text rendering.
 *
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

#include "DrawState.h"

#include "AGGTextRenderer.h"
#include "DrawingMode.h"
#include "PatternHandler.h"
#include "RenderingBuffer.h"
#include "ServerBitmap.h"
#include "ServerFont.h"

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
	  fScanline(NULL),
	  fRasterizer(NULL),
	  fRenderer(NULL),
	  fFontRendererSolid(NULL),
	  fFontRendererBin(NULL),
	  fLineProfile(),
	  fSubpixelPrecise(false),

	  fPenSize(1.0),
	  fClippingRegion(new BRegion()),
	  fValidClipping(false),
	  fDrawingMode(B_OP_COPY),
	  fAlphaSrcMode(B_PIXEL_ALPHA),
//	  fAlphaSrcMode(B_CONSTANT_ALPHA),
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

	_UpdateLineWidth();
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
		fPixelFormat->SetDrawingMode(fDrawingMode, fAlphaSrcMode, fAlphaFncMode);

		fBaseRenderer = new renderer_base(*fPixelFormat);
		// attach our clipping region to the renderer, it keeps a pointer
		fBaseRenderer->set_clipping_region(fClippingRegion);

		// These are the AGG renderes and rasterizes which
		// will be used for stroking paths
#if ALIASED_DRAWING
		fOutlineRenderer = new outline_renderer_type(*fBaseRenderer);
		fOutlineRasterizer = new outline_rasterizer_type(*fOutlineRenderer);
#else
		fOutlineRenderer = new outline_renderer_type(*fBaseRenderer, fLineProfile);
		fOutlineRasterizer = new outline_rasterizer_type(*fOutlineRenderer);

		// attach our line profile to the renderer, it keeps a pointer
		fOutlineRenderer->profile(fLineProfile);
#endif
		// the renderer used for filling paths
		fRenderer = new renderer_type(*fBaseRenderer);
		fRasterizer = new rasterizer_type();
		fScanline = new scanline_type();

#if ALIASED_DRAWING
		fRasterizer->gamma(agg::gamma_threshold(0.5));
#endif

		// These are renderers needed for drawing text
		fFontRendererSolid = new font_renderer_solid_type(*fBaseRenderer);
		fFontRendererBin = new font_renderer_bin_type(*fBaseRenderer);

		_SetRendererColor(fPatternHandler->HighColor().GetColor32());
	}
}

// DetachFromBuffer
void
Painter::DetachFromBuffer()
{
	_MakeEmpty();
}

// SetDrawState
void
Painter::SetDrawState(const DrawState* data)
{
	// NOTE: The custom clipping in "data" is ignored, because it has already been
	// taken into account elsewhere

	// TODO: optimize "context switch" for speed...
	// but for now...
	SetPenSize(data->PenSize());
	SetPenLocation(data->PenLocation());
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

	if (updateDrawingMode)
		_UpdateDrawingMode();

	SetHighColor(data->HighColor().GetColor32());
	SetLowColor(data->LowColor().GetColor32());
}

// #pragma mark -

// ConstrainClipping
void
Painter::ConstrainClipping(const BRegion& region)
{
	*fClippingRegion = region;
	fValidClipping = fClippingRegion->Frame().IsValid();
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
	fPatternHandler->SetLowColor(color);;
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

// SetDrawingMode
void
Painter::SetDrawingMode(drawing_mode mode)
{
	if (fDrawingMode != mode) {
		fDrawingMode = mode;
		_UpdateDrawingMode();
	}
}

// SetBlendingMode
void
Painter::SetBlendingMode(source_alpha alphaSrcMode, alpha_function alphaFncMode)
{
	if (fAlphaSrcMode != alphaSrcMode || fAlphaFncMode != alphaFncMode) {
		fAlphaSrcMode = alphaSrcMode;
		fAlphaFncMode = alphaFncMode;
		if (fDrawingMode == B_OP_ALPHA)
			_UpdateDrawingMode();
	}
}

// SetPattern
void
Painter::SetPattern(const pattern& p)
{
	if (!(p == *fPatternHandler->GetR5Pattern())) {
		fPatternHandler->SetPattern(p);
		_UpdateDrawingMode();
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

// #pragma mark -

// StrokeLine
BRect
Painter::StrokeLine(BPoint a, BPoint b, DrawState* context)
{
	// this happens independent of wether we actually draw something
	context->SetPenLocation(b);
	// NOTE: penlocation should be converted to the local
	// coordinate of the view for which we draw here, after
	// we have been used. Updating it could also be done somewhere
	// else in the app_server code, but DrawString() needs to
	// do this as well, and it is probably hard to calculate
	// the correct location outside of AGGTextRenderer...

	CHECK_CLIPPING

	// "false" means not to do the pixel center offset,
	// because it would mess up our optimized versions
	_Transform(&a, false);
	_Transform(&b, false);

	SetPenSize(context->PenSize());

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

//	SetHighColor(context->highcolor.GetColor32());
//	SetLowColor(context->lowcolor.GetColor32());
//	SetDrawingMode(context->draw_mode);
//	SetBlendingMode(context->alphaSrcMode, context->alphaFncMode);
//	fPatternHandler->SetPattern(context->patt);
	SetDrawState(context);

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

// StrokeLine
BRect
Painter::StrokeLine(BPoint b, DrawState* context)
{
	// TODO: move this function elsewhere
	return StrokeLine(context->PenLocation(), b, context);
}

typedef union {
	uint32	data32;
	int8	data[4];
} color32;

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
			color32 color;
			color.data[0] = c.blue;
			color.data[1] = c.green;
			color.data[2] = c.red;
			color.data[3] = 255;
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
			uint8* dst = fBuffer->row(0);
			uint32 bpr = fBuffer->stride();
			int32 y = (int32)a.y;
			dst += y * bpr;
			int32 x1 = (int32)min_c(a.x, b.x);
			int32 x2 = (int32)max_c(a.x, b.x);
			color32 color;
			color.data[0] = c.blue;
			color.data[1] = c.green;
			color.data[2] = c.red;
			color.data[3] = 255;
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

// StrokePolygon
BRect
Painter::StrokePolygon(const BPoint* ptArray, int32 numPts,
					   bool closed) const
{
	return _DrawPolygon(ptArray, numPts, closed, false);
}

// FillPolygon
BRect
Painter::FillPolygon(const BPoint* ptArray, int32 numPts,
					   bool closed) const
{
	return _DrawPolygon(ptArray, numPts, closed, true);
}

// StrokeBezier
BRect
Painter::StrokeBezier(const BPoint* controlPoints) const
{
	CHECK_CLIPPING

	agg::path_storage curve;

	BPoint p1(controlPoints[0]);
	BPoint p2(controlPoints[1]);
	BPoint p3(controlPoints[2]);
	BPoint p4(controlPoints[3]);
	_Transform(&p1);
	_Transform(&p2);
	_Transform(&p3);
	_Transform(&p4);

	curve.move_to(p1.x, p1.y);
	curve.curve4(p1.x, p1.y,
				 p2.x, p2.y,
				 p3.x, p3.y);


	agg::conv_curve<agg::path_storage> path(curve);

	return _StrokePath(path);
}

// FillBezier
BRect
Painter::FillBezier(const BPoint* controlPoints) const
{
	CHECK_CLIPPING

	agg::path_storage curve;

	BPoint p1(controlPoints[0]);
	BPoint p2(controlPoints[1]);
	BPoint p3(controlPoints[2]);
	BPoint p4(controlPoints[3]);
	_Transform(&p1);
	_Transform(&p2);
	_Transform(&p3);
	_Transform(&p4);

	curve.move_to(p1.x, p1.y);
	curve.curve4(p1.x, p1.y,
				 p2.x, p2.y,
				 p3.x, p3.y);
	curve.close_polygon();

	agg::conv_curve<agg::path_storage> path(curve);

	return _FillPath(path);
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

	agg::path_storage path;
	for (int32 i = 0; i < opCount; i++) {
		switch (opList[i] & 0xFF000000) {
			case OP_LINETO: {
				int32 count = opList[i] & 0x00FFFFFF;
				while (count--) {
					path.line_to(points->x, points->y);
					points++;
				}
				break;
			}
			case (OP_MOVETO | OP_LINETO): {
				int32 count = opList[i] & 0x00FFFFFF;
				path.move_to(points->x, points->y);
				points++;
				// execute "line to(s)"
				while (count--) {
					path.line_to(points->x, points->y);
					points++;
				}
				break;
			}
			case OP_BEZIERTO: {
				int32 count = opList[i] & 0x00FFFFFF;
				while (count) {
					path.curve4(points[0].x, points[0].y,
								points[1].x, points[1].y,
								points[2].x, points[2].y);
					points += 3;
					count -= 3;
				}
				break;
			}
			case (OP_MOVETO | OP_BEZIERTO): {
				int32 count = opList[i] & 0x00FFFFFF;
				// execute "move to"
				path.move_to(points->x, points->y);
				points++;
				// execute "bezier to(s)"
				while (count) {
					path.curve4(points[0].x, points[0].y,
								points[1].x, points[1].y,
								points[2].x, points[2].y);
					points += 3;
					count -= 3;
				}
				break;
			}
			case OP_CLOSE:
			case OP_CLOSE | OP_LINETO | OP_BEZIERTO: {
				path.close_polygon();
				break;
			}
		}
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
		color32 color;
		color.data[0] = c.blue;
		color.data[1] = c.green;
		color.data[2] = c.red;
		color.data[3] = 255;
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
					uint32* handle = (uint32*)(offset + y1 * bpr);
					for (int32 x = x1; x <= x2; x++) {
						*handle++ = color.data32;
					}
				}
			}
		} while (fBaseRenderer->next_clip_box());
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
	
		agg::render_scanlines(*fRasterizer, *fScanline, *fRenderer);
	
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
									
// StrokeEllipse
BRect
Painter::StrokeEllipse(BPoint center, float xRadius, float yRadius) const
{
	return _DrawEllipse(center, xRadius, yRadius, false);
}

// FillEllipse
BRect
Painter::FillEllipse(BPoint center, float xRadius, float yRadius) const
{
	return _DrawEllipse(center, xRadius, yRadius, true);
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

// DrawChar
BRect
Painter::DrawChar(char aChar)
{
	// TODO: to be moved elsewhere
	return DrawChar(aChar, fPenLocation);
}

// DrawChar
BRect
Painter::DrawChar(char aChar, BPoint baseLine)
{
	// TODO: to be moved elsewhere
	char wrapper[2];
	wrapper[0] = aChar;
	wrapper[1] = 0;
	return DrawString(wrapper, 1, baseLine);
}

// DrawString
BRect
Painter::DrawString(const char* utf8String, uint32 length,
					const escapement_delta* delta)
{
	// TODO: to be moved elsewhere
	return DrawString(utf8String, length, fPenLocation, delta);
}

// DrawString
BRect
Painter::DrawString(const char* utf8String, uint32 length,
					BPoint baseLine, const escapement_delta* delta)
{
	CHECK_CLIPPING

	BRect bounds(0.0, 0.0, -1.0, -1.0);

	SetPattern(B_SOLID_HIGH);

	if (fBuffer) {
		bounds = fTextRenderer->RenderString(utf8String,
											 length,
											 fFontRendererSolid,
											 fFontRendererBin,
											 baseLine,
											 fClippingRegion->Frame(),
											 false,
											 &fPenLocation, 
											 delta);
	}
	return _Clipped(bounds);
}

// DrawString
BRect
Painter::DrawString(const char* utf8String, const escapement_delta* delta)
{
	// TODO: to be moved elsewhere
	return DrawString(utf8String, strlen(utf8String), fPenLocation, delta);
}

// DrawString
BRect
Painter::DrawString(const char* utf8String, BPoint baseLine,
					const escapement_delta* delta)
{
	// TODO: to be moved elsewhere
	return DrawString(utf8String, strlen(utf8String), baseLine, delta);
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

// BoundingBox
BRect
Painter::BoundingBox(const char* utf8String, uint32 length,
					 const BPoint& baseLine, BPoint* penLocation,
					 const escapement_delta* delta) const
{
	static BRect dummy;
	return fTextRenderer->RenderString(utf8String,
									   length,
									   fFontRendererSolid,
									   fFontRendererBin,
									   baseLine, dummy, true, penLocation,
									   delta);
}

// StringWidth
float
Painter::StringWidth(const char* utf8String, uint32 length) const
{
	return fTextRenderer->StringWidth(utf8String, length);
}

// #pragma mark -

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

	delete fOutlineRenderer;
	fOutlineRenderer = NULL;

	delete fOutlineRasterizer;
	fOutlineRasterizer = NULL;

	delete fScanline;
	fScanline = NULL;

	delete fRasterizer;
	fRasterizer = NULL;

	delete fRenderer;
	fRenderer = NULL;

	delete fFontRendererSolid;
	fFontRendererSolid = NULL;

	delete fFontRendererBin;
	fFontRendererBin = NULL;
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
Painter::_UpdateDrawingMode()
{
	// The AGG renderers have their own color setting, however
	// almost all drawing mode classes ignore the color given
	// by the AGG renderer and use the colors from the PatternHandler
	// instead. If we have a B_SOLID_* pattern, we can actually use
	// the color in the renderer and special versions of drawing modes
	// that don't use PatternHandler and are more efficient. This
	// has been implemented for B_OP_COPY as of now, the last parameter
	// to DrawingModeFor() is a flag if a special "solid" drawing mode
	// should be used if available. In this case, _SetRendererColor()
	// has to be called so that all internal colors in the renderes
	// are up to date for use by the solid drawing mode version.
	if (fPixelFormat) {
		if (fPatternHandler->IsSolidHigh()) {
// TODO: fix me! is already set in SetHighColor()
			_SetRendererColor(fPatternHandler->HighColor().GetColor32());
		} else if (fPatternHandler->IsSolidLow()) {
// TODO: fix me! is already set in SetLowColor()
			_SetRendererColor(fPatternHandler->LowColor().GetColor32());
		}
		fPixelFormat->SetDrawingMode(fDrawingMode, fAlphaSrcMode, fAlphaFncMode);
	}
		
}

// _SetRendererColor
void
Painter::_SetRendererColor(const rgb_color& color) const
{
	if (fOutlineRenderer)
#if ALIASED_DRAWING
		fOutlineRenderer->line_color(agg::rgba(color.red / 255.0,
											   color.green / 255.0,
											   color.blue / 255.0));
#else
		fOutlineRenderer->color(agg::rgba(color.red / 255.0,
										  color.green / 255.0,
										  color.blue / 255.0));
#endif
	if (fRenderer)
		fRenderer->color(agg::rgba(color.red / 255.0,
								   color.green / 255.0,
								   color.blue / 255.0));
	if (fFontRendererSolid)
		fFontRendererSolid->color(agg::rgba(color.red / 255.0,
											color.green / 255.0,
											color.blue / 255.0));
	if (fFontRendererBin)
		fFontRendererBin->color(agg::rgba(color.red / 255.0,
										  color.green / 255.0,
										  color.blue / 255.0));

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

// _DrawEllipse
inline BRect
Painter::_DrawEllipse(BPoint center, float xRadius, float yRadius,
					  bool fill) const
{
	CHECK_CLIPPING

	// TODO: I think the conversion and the offset of
	// pixel centers might not be correct here, and it
	// might even be necessary to treat Fill and Stroke
	// differently, as with Fill-/StrokeRect().
	_Transform(&center);

	int32 divisions = (int32)max_c(12, (xRadius + yRadius + 2 * fPenSize) * PI / 2);

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

		agg::render_scanlines(*fRasterizer, *fScanline, *fRenderer);

		// reset to default
		fRasterizer->filling_rule(agg::fill_non_zero);

		return _Clipped(_BoundingBox(outer));
	}
}

// _DrawPolygon
inline BRect
Painter::_DrawPolygon(const BPoint* ptArray, int32 numPts,
					  bool closed, bool fill) const
{
	CHECK_CLIPPING

	if (numPts > 0) {

		agg::path_storage path;
		BPoint point = _Transform(*ptArray);
		path.move_to(point.x, point.y);

		for (int32 i = 1; i < numPts; i++) {
			ptArray++;
			point = _Transform(*ptArray);
			path.line_to(point.x, point.y);
		}

		if (closed)
			path.close_polygon();

		if (fill)
			return _FillPath(path);
		else
			return _StrokePath(path);
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}

// _DrawBitmap
void
Painter::_DrawBitmap(const agg::rendering_buffer& srcBuffer, color_space format,
					 BRect actualBitmapRect, BRect bitmapRect, BRect viewRect) const
{
	switch (format) {
		case B_RGB32:
		case B_RGBA32:
			_DrawBitmap32(srcBuffer, actualBitmapRect, bitmapRect, viewRect);
			break;
		default:
//printf("Painter::_DrawBitmap() - non-native colorspace: %d\n", format);
			// TODO: this is only a temporary implementation,
			// to really handle other colorspaces, one would
			// rather do the conversion with much less overhead,
			// for example in the nn filter (hm), or in the
			// scanline generator (better)
			BBitmap temp(actualBitmapRect, B_BITMAP_NO_SERVER_LINK, B_RGB32);
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
				_DrawBitmap32(convertedBuffer, actualBitmapRect, bitmapRect, viewRect);
			} else {
printf("Painter::_DrawBitmap() - colorspace conversion failed: %s\n", strerror(err));
			}
			break;
	}
}

// _DrawBitmap32
void
Painter::_DrawBitmap32(const agg::rendering_buffer& srcBuffer,
					   BRect actualBitmapRect, BRect bitmapRect, BRect viewRect) const
{
typedef agg::span_allocator<agg::rgba8> span_alloc_type;

// AGG pipeline
typedef agg::span_interpolator_linear<> interpolator_type;
typedef agg::span_image_filter_rgba32_nn<agg::order_bgra32,
										 interpolator_type> scaled_span_gen_type;
typedef agg::renderer_scanline_aa<renderer_base, scaled_span_gen_type> scaled_image_renderer_type;


	if (bitmapRect.IsValid() && bitmapRect.Intersects(actualBitmapRect)
		&& viewRect.IsValid()) {

		// compensate for the lefttop offset the actualBitmapRect might have
// NOTE: I have no clue why enabling the next call gives a wrong result!
// According to the BeBook, bitmapRect is supposed to be in native
// bitmap space! Disabling this call makes it look like the bitmap bounds are
// assumed to have a left/top coord of 0,0 at all times. This is simply not true.
//		bitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);
		actualBitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);

		// calculate the scaling
		double xScale = (viewRect.Width() + 1) / (bitmapRect.Width() + 1);
		double yScale = (viewRect.Height() + 1) / (bitmapRect.Height() + 1);

		if (xScale == 0.0 || yScale == 0.0)
			return;

		// constrain rect to passed bitmap bounds
		// and transfer the changes to the viewRect
		if (bitmapRect.left < actualBitmapRect.left) {
			float diff = actualBitmapRect.left - bitmapRect.left;
			viewRect.left += diff * xScale;
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

		float xOffset = viewRect.left - (bitmapRect.left * xScale);
		float yOffset = viewRect.top - (bitmapRect.top * yScale);

		agg::trans_affine srcMatrix;
//		srcMatrix *= agg::trans_affine_translation(-actualBitmapRect.left, -actualBitmapRect.top);

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
		agg::render_scanlines(*fRasterizer, *fScanline, *fRenderer);
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
	agg::render_scanlines(*fRasterizer, *fScanline, *fRenderer);

	return _Clipped(_BoundingBox(clippedPath));
}

