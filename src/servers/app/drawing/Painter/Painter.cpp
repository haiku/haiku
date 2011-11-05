/*
 * Copyright 2009, Christian Packmann.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * Copyright 2005-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


/*!	API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
	rendering pipe-lines for stroke, fills, bitmap and text rendering.
*/


#include "Painter.h"

#include <new>

#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Region.h>
#include <String.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>

#include <ShapePrivate.h>

#include <agg_bezier_arc.h>
#include <agg_bounding_rect.h>
#include <agg_conv_clip_polygon.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>
#include <agg_ellipse.h>
#include <agg_image_accessors.h>
#include <agg_path_storage.h>
#include <agg_pixfmt_rgba.h>
#include <agg_rounded_rect.h>
#include <agg_span_allocator.h>
#include <agg_span_image_filter_rgba.h>
#include <agg_span_interpolator_linear.h>

#include "drawing_support.h"

#include "DrawState.h"

#include <AutoDeleter.h>
#include <View.h>

#include "DrawingMode.h"
#include "GlobalSubpixelSettings.h"
#include "PatternHandler.h"
#include "RenderingBuffer.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "SystemPalette.h"

#include "AppServer.h"

using std::nothrow;

#undef TRACE
// #define TRACE_PAINTER
#ifdef TRACE_PAINTER
#	define TRACE(x...)		printf(x)
#else
#	define TRACE(x...)
#endif

//#define TRACE_GRADIENTS
#ifdef TRACE_GRADIENTS
#	include <OS.h>
#	define GTRACE(x...)		debug_printf(x)
#else
#	define GTRACE(x...)
#endif


#define CHECK_CLIPPING	if (!fValidClipping) return BRect(0, 0, -1, -1);
#define CHECK_CLIPPING_NO_RETURN	if (!fValidClipping) return;

// Defines for SIMD support.
#define APPSERVER_SIMD_MMX	(1 << 0)
#define APPSERVER_SIMD_SSE	(1 << 1)

// Prototypes for assembler routines
extern "C" {
	void bilinear_scale_xloop_mmxsse(const uint8* src, void* dst,
		void* xWeights, uint32 xmin, uint32 xmax, uint32 wTop, uint32 srcBPR);
}

static uint32 detect_simd();

static uint32 sSIMDFlags = detect_simd();


/*!	Detect SIMD flags for use in AppServer. Checks all CPUs in the system
	and chooses the minimum supported set of instructions.
*/
static uint32
detect_simd()
{
#if __INTEL__
	// Only scan CPUs for which we are certain the SIMD flags are properly
	// defined.
	const char* vendorNames[] = {
		"GenuineIntel",
		"AuthenticAMD",
		"CentaurHauls", // Via CPUs, MMX and SSE support
		"RiseRiseRise", // should be MMX-only
		"CyrixInstead", // MMX-only, but custom MMX extensions
		"GenuineTMx86", // MMX and SSE
		0
	};

	system_info systemInfo;
	if (get_system_info(&systemInfo) != B_OK)
		return 0;

	// We start out with all flags set and end up with only those flags
	// supported across all CPUs found.
	uint32 systemSIMD = 0xffffffff;

	for (int32 cpu = 0; cpu < systemInfo.cpu_count; cpu++) {
		cpuid_info cpuInfo;
		get_cpuid(&cpuInfo, 0, cpu);

		// Get the vendor string and terminate it manually
		char vendor[13];
		memcpy(vendor, cpuInfo.eax_0.vendor_id, 12);
		vendor[12] = 0;

		bool vendorFound = false;
		for (uint32 i = 0; vendorNames[i] != 0; i++) {
			if (strcmp(vendor, vendorNames[i]) == 0)
				vendorFound = true;
		}

		uint32 cpuSIMD = 0;
		uint32 maxStdFunc = cpuInfo.regs.eax;
		if (vendorFound && maxStdFunc >= 1) {
			get_cpuid(&cpuInfo, 1, 0);
			uint32 edx = cpuInfo.regs.edx;
			if (edx & (1 << 23))
				cpuSIMD |= APPSERVER_SIMD_MMX;
			if (edx & (1 << 25))
				cpuSIMD |= APPSERVER_SIMD_SSE;
		} else {
			// no flags can be identified
			cpuSIMD = 0;
		}
		systemSIMD &= cpuSIMD;
	}
	return systemSIMD;
#else	// !__INTEL__
	return 0;
#endif
}


// #pragma mark -


Painter::Painter()
	:
	fBuffer(),
	fPixelFormat(fBuffer, &fPatternHandler),
	fBaseRenderer(fPixelFormat),
	fUnpackedScanline(),
	fPackedScanline(),
	fSubpixPackedScanline(),
	fSubpixUnpackedScanline(),
	fSubpixRasterizer(),
	fRasterizer(),
	fSubpixRenderer(fBaseRenderer),
	fRenderer(fBaseRenderer),
	fRendererBin(fBaseRenderer),

	fPath(),
	fCurve(fPath),

	fSubpixelPrecise(false),
	fValidClipping(false),
	fDrawingText(false),
	fAttached(false),

	fPenSize(1.0),
	fClippingRegion(NULL),
	fDrawingMode(B_OP_COPY),
	fAlphaSrcMode(B_PIXEL_ALPHA),
	fAlphaFncMode(B_ALPHA_OVERLAY),
	fLineCapMode(B_BUTT_CAP),
	fLineJoinMode(B_MITER_JOIN),
	fMiterLimit(B_DEFAULT_MITER_LIMIT),

	fPatternHandler(),
	fTextRenderer(fSubpixRenderer, fRenderer, fRendererBin, fUnpackedScanline,
		fSubpixUnpackedScanline, fSubpixRasterizer)
{
	fPixelFormat.SetDrawingMode(fDrawingMode, fAlphaSrcMode, fAlphaFncMode,
		false);

#if ALIASED_DRAWING
	fRasterizer.gamma(agg::gamma_threshold(0.5));
	fSubpixRasterizer.gamma(agg:gamma_threshold(0.5));
#endif
}


// destructor
Painter::~Painter()
{
}


// #pragma mark -


// AttachToBuffer
void
Painter::AttachToBuffer(RenderingBuffer* buffer)
{
	if (buffer && buffer->InitCheck() >= B_OK
		&& (buffer->ColorSpace() == B_RGBA32
			|| buffer->ColorSpace() == B_RGB32)) {
		// TODO: implement drawing on B_RGB24, B_RGB15, B_RGB16,
		// B_CMAP8 and B_GRAY8 :-[
		// (if ever we want to support some devices where this gives
		// a great speed up, right now it seems fine, even in emulation)

		fBuffer.attach((uint8*)buffer->Bits(),
			buffer->Width(), buffer->Height(), buffer->BytesPerRow());

		fAttached = true;
		fValidClipping = fClippingRegion != NULL
			&& fClippingRegion->Frame().IsValid();

		// These are the AGG renderes and rasterizes which
		// will be used for stroking paths

		_SetRendererColor(fPatternHandler.HighColor());
	}
}


// DetachFromBuffer
void
Painter::DetachFromBuffer()
{
	fBuffer.attach(NULL, 0, 0, 0);
	fAttached = false;
	fValidClipping = false;
}


// Bounds
BRect
Painter::Bounds() const
{
	return BRect(0, 0, fBuffer.width() - 1, fBuffer.height() - 1);
}


// #pragma mark -


// SetDrawState
void
Painter::SetDrawState(const DrawState* data, int32 xOffset, int32 yOffset)
{
	// NOTE: The custom clipping in "data" is ignored, because it has already
	// been taken into account elsewhere

	// NOTE: Usually this function is only used when the "current view"
	// is switched in the ServerWindow and after the decorator has drawn
	// and messed up the state. For other graphics state changes, the
	// Painter methods are used directly, so this function is much less
	// speed critical than it used to be.

	SetPenSize(data->PenSize());

	SetFont(data);

	fSubpixelPrecise = data->SubPixelPrecise();

	// any of these conditions means we need to use a different drawing
	// mode instance
	bool updateDrawingMode
		= !(data->GetPattern() == fPatternHandler.GetPattern())
			|| data->GetDrawingMode() != fDrawingMode
			|| (data->GetDrawingMode() == B_OP_ALPHA
				&& (data->AlphaSrcMode() != fAlphaSrcMode
					|| data->AlphaFncMode() != fAlphaFncMode));

	fDrawingMode = data->GetDrawingMode();
	fAlphaSrcMode = data->AlphaSrcMode();
	fAlphaFncMode = data->AlphaFncMode();
	fPatternHandler.SetPattern(data->GetPattern());
	fPatternHandler.SetOffsets(xOffset, yOffset);
	fLineCapMode = data->LineCapMode();
	fLineJoinMode = data->LineJoinMode();
	fMiterLimit = data->MiterLimit();

	// adopt the color *after* the pattern is set
	// to set the renderers to the correct color
	SetHighColor(data->HighColor());
	SetLowColor(data->LowColor());

	if (updateDrawingMode || fPixelFormat.UsesOpCopyForText())
		_UpdateDrawingMode();
}


// #pragma mark - state


// ConstrainClipping
void
Painter::ConstrainClipping(const BRegion* region)
{
	fClippingRegion = region;
	fBaseRenderer.set_clipping_region(const_cast<BRegion*>(region));
	fValidClipping = region->Frame().IsValid() && fAttached;

	if (fValidClipping) {
		clipping_rect cb = fClippingRegion->FrameInt();
		fRasterizer.clip_box(cb.left, cb.top, cb.right + 1, cb.bottom + 1);
		fSubpixRasterizer.clip_box(cb.left, cb.top, cb.right + 1, cb.bottom + 1);
	}
}


// SetHighColor
void
Painter::SetHighColor(const rgb_color& color)
{
	if (fPatternHandler.HighColor() == color)
		return;
	fPatternHandler.SetHighColor(color);
	if (*(fPatternHandler.GetR5Pattern()) == B_SOLID_HIGH)
		_SetRendererColor(color);
}


// SetLowColor
void
Painter::SetLowColor(const rgb_color& color)
{
	fPatternHandler.SetLowColor(color);
	if (*(fPatternHandler.GetR5Pattern()) == B_SOLID_LOW)
		_SetRendererColor(color);
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
Painter::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc)
{
	if (fAlphaSrcMode != srcAlpha || fAlphaFncMode != alphaFunc) {
		fAlphaSrcMode = srcAlpha;
		fAlphaFncMode = alphaFunc;
		if (fDrawingMode == B_OP_ALPHA)
			_UpdateDrawingMode();
	}
}


// SetPenSize
void
Painter::SetPenSize(float size)
{
	fPenSize = size;
}


// SetStrokeMode
void
Painter::SetStrokeMode(cap_mode lineCap, join_mode joinMode, float miterLimit)
{
	fLineCapMode = lineCap;
	fLineJoinMode = joinMode;
	fMiterLimit = miterLimit;
}


// SetPattern
void
Painter::SetPattern(const pattern& p, bool drawingText)
{
	if (!(p == *fPatternHandler.GetR5Pattern()) || drawingText != fDrawingText) {
		fPatternHandler.SetPattern(p);
		fDrawingText = drawingText;
		_UpdateDrawingMode(fDrawingText);

		// update renderer color if necessary
		if (fPatternHandler.IsSolidHigh()) {
			// pattern was not solid high before
			_SetRendererColor(fPatternHandler.HighColor());
		} else if (fPatternHandler.IsSolidLow()) {
			// pattern was not solid low before
			_SetRendererColor(fPatternHandler.LowColor());
		}
	}
}


// SetFont
void
Painter::SetFont(const ServerFont& font)
{
	fTextRenderer.SetFont(font);
	fTextRenderer.SetAntialiasing(!(font.Flags() & B_DISABLE_ANTIALIASING));
}


// SetFont
void
Painter::SetFont(const DrawState* state)
{
	fTextRenderer.SetFont(state->Font());
	fTextRenderer.SetAntialiasing(!state->ForceFontAliasing()
		&& (state->Font().Flags() & B_DISABLE_ANTIALIASING) == 0);
}


// #pragma mark - drawing


// StrokeLine
void
Painter::StrokeLine(BPoint a, BPoint b)
{
	CHECK_CLIPPING_NO_RETURN

	// "false" means not to do the pixel center offset,
	// because it would mess up our optimized versions
	_Transform(&a, false);
	_Transform(&b, false);

	// first, try an optimized version
	if (fPenSize == 1.0
		&& (fDrawingMode == B_OP_COPY || fDrawingMode == B_OP_OVER)) {
		pattern pat = *fPatternHandler.GetR5Pattern();
		if (pat == B_SOLID_HIGH
			&& StraightLine(a, b, fPatternHandler.HighColor())) {
			return;
		} else if (pat == B_SOLID_LOW
			&& StraightLine(a, b, fPatternHandler.LowColor())) {
			return;
		}
	}

	fPath.remove_all();

	if (a == b) {
		// special case dots
		if (fPenSize == 1.0 && !fSubpixelPrecise) {
			if (fClippingRegion->Contains(a)) {
				fPixelFormat.blend_pixel((int)a.x, (int)a.y, fRenderer.color(),
					255);
			}
		} else {
			fPath.move_to(a.x, a.y);
			fPath.line_to(a.x + 1, a.y);
			fPath.line_to(a.x + 1, a.y + 1);
			fPath.line_to(a.x, a.y + 1);

			_FillPath(fPath);
		}
	} else {
		// do the pixel center offset here
		// tweak ends to "include" the pixel at the index,
		// we need to do this in order to produce results like R5,
		// where coordinates were inclusive
		if (!fSubpixelPrecise) {
			bool centerOnLine = fmodf(fPenSize, 2.0) != 0.0;
			if (a.x == b.x) {
				// shift to pixel center vertically
				if (centerOnLine) {
					a.x += 0.5;
					b.x += 0.5;
				}
				// extend on bottom end
				if (a.y < b.y)
					b.y++;
				else
					a.y++;
			} else if (a.y == b.y) {
				if (centerOnLine) {
					// shift to pixel center horizontally
					a.y += 0.5;
					b.y += 0.5;
				}
				// extend on right end
				if (a.x < b.x)
					b.x++;
				else
					a.x++;
			} else {
				// do this regardless of pensize
				if (a.x < b.x)
					b.x++;
				else
					a.x++;
				if (a.y < b.y)
					b.y++;
				else
					a.y++;
			}
		}

		fPath.move_to(a.x, a.y);
		fPath.line_to(b.x, b.y);

		_StrokePath(fPath);
	}
}


// StraightLine
bool
Painter::StraightLine(BPoint a, BPoint b, const rgb_color& c) const
{
	if (!fValidClipping)
		return false;

	if (a.x == b.x) {
		// vertical
		uint8* dst = fBuffer.row_ptr(0);
		uint32 bpr = fBuffer.stride();
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
		fBaseRenderer.first_clip_box();
		do {
			if (fBaseRenderer.xmin() <= x &&
				fBaseRenderer.xmax() >= x) {
				int32 i = max_c(fBaseRenderer.ymin(), y1);
				int32 end = min_c(fBaseRenderer.ymax(), y2);
				uint8* handle = dst + i * bpr;
				for (; i <= end; i++) {
					*(uint32*)handle = color.data32;
					handle += bpr;
				}
			}
		} while (fBaseRenderer.next_clip_box());

		return true;
	}

	if (a.y == b.y) {
		// horizontal
		int32 y = (int32)a.y;
		if (y < 0 || y >= (int32)fBuffer.height())
			return true;

		uint8* dst = fBuffer.row_ptr(y);
		int32 x1 = (int32)min_c(a.x, b.x);
		int32 x2 = (int32)max_c(a.x, b.x);
		pixel32 color;
		color.data8[0] = c.blue;
		color.data8[1] = c.green;
		color.data8[2] = c.red;
		color.data8[3] = 255;
		// draw a line, iterate over clipping boxes
		fBaseRenderer.first_clip_box();
		do {
			if (fBaseRenderer.ymin() <= y &&
				fBaseRenderer.ymax() >= y) {
				int32 i = max_c(fBaseRenderer.xmin(), x1);
				int32 end = min_c(fBaseRenderer.xmax(), x2);
				uint32* handle = (uint32*)(dst + i * 4);
				for (; i <= end; i++) {
					*handle++ = color.data32;
				}
			}
		} while (fBaseRenderer.next_clip_box());

		return true;
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


// FillTriangle
BRect
Painter::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
	const BGradient& gradient) const
{
	CHECK_CLIPPING

	_Transform(&pt1);
	_Transform(&pt2);
	_Transform(&pt3);

	fPath.remove_all();

	fPath.move_to(pt1.x, pt1.y);
	fPath.line_to(pt2.x, pt2.y);
	fPath.line_to(pt3.x, pt3.y);

	fPath.close_polygon();

	return _FillPath(fPath, gradient);
}


// DrawPolygon
BRect
Painter::DrawPolygon(BPoint* p, int32 numPts, bool filled, bool closed) const
{
	CHECK_CLIPPING

	if (numPts > 0) {
		fPath.remove_all();

		_Transform(p);
		fPath.move_to(p->x, p->y);

		for (int32 i = 1; i < numPts; i++) {
			p++;
			_Transform(p);
			fPath.line_to(p->x, p->y);
		}

		if (closed)
			fPath.close_polygon();

		if (filled)
			return _FillPath(fPath);

		return _StrokePath(fPath);
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}


// FillPolygon
BRect
Painter::FillPolygon(BPoint* p, int32 numPts, const BGradient& gradient,
	bool closed) const
{
	CHECK_CLIPPING

	if (numPts > 0) {
		fPath.remove_all();

		_Transform(p);
		fPath.move_to(p->x, p->y);

		for (int32 i = 1; i < numPts; i++) {
			p++;
			_Transform(p);
			fPath.line_to(p->x, p->y);
		}

		if (closed)
			fPath.close_polygon();

		return _FillPath(fPath, gradient);
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}


// DrawBezier
BRect
Painter::DrawBezier(BPoint* p, bool filled) const
{
	CHECK_CLIPPING

	fPath.remove_all();

	_Transform(&(p[0]));
	_Transform(&(p[1]));
	_Transform(&(p[2]));
	_Transform(&(p[3]));

	fPath.move_to(p[0].x, p[0].y);
	fPath.curve4(p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);

	if (filled) {
		fPath.close_polygon();
		return _FillPath(fCurve);
	}

	return _StrokePath(fCurve);
}


// FillBezier
BRect
Painter::FillBezier(BPoint* p, const BGradient& gradient) const
{
	CHECK_CLIPPING

	fPath.remove_all();

	_Transform(&(p[0]));
	_Transform(&(p[1]));
	_Transform(&(p[2]));
	_Transform(&(p[3]));

	fPath.move_to(p[0].x, p[0].y);
	fPath.curve4(p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);

	fPath.close_polygon();
	return _FillPath(fCurve, gradient);
}


static void
iterate_shape_data(agg::path_storage& path,
	const int32& opCount, const uint32* opList,
	const int32& ptCount, const BPoint* points,
	const BPoint& viewToScreenOffset, float viewScale)
{
	// TODO: if shapes are ever used more heavily in Haiku,
	// it would be nice to use BShape data directly (write
	// an AGG "VertexSource" adaptor)
	path.remove_all();
	for (int32 i = 0; i < opCount; i++) {
		uint32 op = opList[i] & 0xFF000000;
		if (op & OP_MOVETO) {
			path.move_to(
				points->x * viewScale + viewToScreenOffset.x,
				points->y * viewScale + viewToScreenOffset.y);
			points++;
		}

		if (op & OP_LINETO) {
			int32 count = opList[i] & 0x00FFFFFF;
			while (count--) {
				path.line_to(
					points->x * viewScale + viewToScreenOffset.x,
					points->y * viewScale + viewToScreenOffset.y);
				points++;
			}
		}

		if (op & OP_BEZIERTO) {
			int32 count = opList[i] & 0x00FFFFFF;
			while (count) {
				path.curve4(
					points[0].x * viewScale + viewToScreenOffset.x,
					points[0].y * viewScale + viewToScreenOffset.y,
					points[1].x * viewScale + viewToScreenOffset.x,
					points[1].y * viewScale + viewToScreenOffset.y,
					points[2].x * viewScale + viewToScreenOffset.x,
					points[2].y * viewScale + viewToScreenOffset.y);
				points += 3;
				count -= 3;
			}
		}

		if ((op & OP_LARGE_ARC_TO_CW) || (op & OP_LARGE_ARC_TO_CCW)
			|| (op & OP_SMALL_ARC_TO_CW) || (op & OP_SMALL_ARC_TO_CCW)) {
			int32 count = opList[i] & 0x00FFFFFF;
			while (count) {
				path.arc_to(
					points[0].x * viewScale,
					points[0].y * viewScale,
					points[1].x,
					op & (OP_LARGE_ARC_TO_CW | OP_LARGE_ARC_TO_CCW),
					op & (OP_SMALL_ARC_TO_CW | OP_LARGE_ARC_TO_CW),
					points[2].x * viewScale + viewToScreenOffset.x,
					points[2].y * viewScale + viewToScreenOffset.y);
				points += 3;
				count -= 3;
			}
		}

		if (op & OP_CLOSE)
			path.close_polygon();
	}
}


// DrawShape
BRect
Painter::DrawShape(const int32& opCount, const uint32* opList,
	const int32& ptCount, const BPoint* points, bool filled,
	const BPoint& viewToScreenOffset, float viewScale) const
{
	CHECK_CLIPPING

	iterate_shape_data(fPath, opCount, opList, ptCount, points,
		viewToScreenOffset, viewScale);

	if (filled)
		return _FillPath(fCurve);

	return _StrokePath(fCurve);
}


// FillShape
BRect
Painter::FillShape(const int32& opCount, const uint32* opList,
	const int32& ptCount, const BPoint* points, const BGradient& gradient,
	const BPoint& viewToScreenOffset, float viewScale) const
{
	CHECK_CLIPPING

	iterate_shape_data(fPath, opCount, opList, ptCount, points,
		viewToScreenOffset, viewScale);

	return _FillPath(fCurve, gradient);
}


// StrokeRect
BRect
Painter::StrokeRect(const BRect& r) const
{
	CHECK_CLIPPING

	BPoint a(r.left, r.top);
	BPoint b(r.right, r.bottom);
	_Transform(&a, false);
	_Transform(&b, false);

	// first, try an optimized version
	if (fPenSize == 1.0 &&
		(fDrawingMode == B_OP_COPY || fDrawingMode == B_OP_OVER)) {
		pattern p = *fPatternHandler.GetR5Pattern();
		if (p == B_SOLID_HIGH) {
			BRect rect(a, b);
			StrokeRect(rect, fPatternHandler.HighColor());
			return _Clipped(rect);
		} else if (p == B_SOLID_LOW) {
			BRect rect(a, b);
			StrokeRect(rect, fPatternHandler.LowColor());
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

	fPath.remove_all();
	fPath.move_to(a.x, a.y);
	if (a.x == b.x || a.y == b.y) {
		// special case rects with one pixel height or width
		fPath.line_to(b.x, b.y);
	} else {
		fPath.line_to(b.x, a.y);
		fPath.line_to(b.x, b.y);
		fPath.line_to(a.x, b.y);
	}
	fPath.close_polygon();

	return _StrokePath(fPath);
}


// StrokeRect
void
Painter::StrokeRect(const BRect& r, const rgb_color& c) const
{
	StraightLine(BPoint(r.left, r.top), BPoint(r.right - 1, r.top), c);
	StraightLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom - 1), c);
	StraightLine(BPoint(r.right, r.bottom), BPoint(r.left + 1, r.bottom), c);
	StraightLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top + 1), c);
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
		pattern p = *fPatternHandler.GetR5Pattern();
		if (p == B_SOLID_HIGH) {
			BRect rect(a, b);
			FillRect(rect, fPatternHandler.HighColor());
			return _Clipped(rect);
		} else if (p == B_SOLID_LOW) {
			BRect rect(a, b);
			FillRect(rect, fPatternHandler.LowColor());
			return _Clipped(rect);
		}
	}
	if (fDrawingMode == B_OP_ALPHA && fAlphaFncMode == B_ALPHA_OVERLAY) {
		pattern p = *fPatternHandler.GetR5Pattern();
		if (p == B_SOLID_HIGH) {
			BRect rect(a, b);
			_BlendRect32(rect, fPatternHandler.HighColor());
			return _Clipped(rect);
		} else if (p == B_SOLID_LOW) {
			rgb_color c = fPatternHandler.LowColor();
			if (fAlphaSrcMode == B_CONSTANT_ALPHA)
				c.alpha = fPatternHandler.HighColor().alpha;
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

	fPath.remove_all();
	fPath.move_to(a.x, a.y);
	fPath.line_to(b.x, a.y);
	fPath.line_to(b.x, b.y);
	fPath.line_to(a.x, b.y);
	fPath.close_polygon();

	return _FillPath(fPath);
}


// FillRect
BRect
Painter::FillRect(const BRect& r, const BGradient& gradient) const
{
	CHECK_CLIPPING

	// support invalid rects
	BPoint a(min_c(r.left, r.right), min_c(r.top, r.bottom));
	BPoint b(max_c(r.left, r.right), max_c(r.top, r.bottom));
	_Transform(&a, false);
	_Transform(&b, false);

	// first, try an optimized version
	if (gradient.GetType() == BGradient::TYPE_LINEAR
		&& (fDrawingMode == B_OP_COPY || fDrawingMode == B_OP_OVER)) {
		const BGradientLinear* linearGradient
			= dynamic_cast<const BGradientLinear*>(&gradient);
		if (linearGradient->Start().x == linearGradient->End().x
			// TODO: Remove this second check once the optimized method
			// handled "upside down" gradients as well...
			&& linearGradient->Start().y <= linearGradient->End().y) {
			// a vertical gradient
			BRect rect(a, b);
			FillRectVerticalGradient(rect, *linearGradient);
			return _Clipped(rect);
		}
	}

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	b.x += 1.0;
	b.y += 1.0;

	fPath.remove_all();
	fPath.move_to(a.x, a.y);
	fPath.line_to(b.x, a.y);
	fPath.line_to(b.x, b.y);
	fPath.line_to(a.x, b.y);
	fPath.close_polygon();

	return _FillPath(fPath, gradient);
}


// FillRect
void
Painter::FillRect(const BRect& r, const rgb_color& c) const
{
	if (!fValidClipping)
		return;

	uint8* dst = fBuffer.row_ptr(0);
	uint32 bpr = fBuffer.stride();
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
	fBaseRenderer.first_clip_box();
	do {
		int32 x1 = max_c(fBaseRenderer.xmin(), left);
		int32 x2 = min_c(fBaseRenderer.xmax(), right);
		if (x1 <= x2) {
			int32 y1 = max_c(fBaseRenderer.ymin(), top);
			int32 y2 = min_c(fBaseRenderer.ymax(), bottom);
			uint8* offset = dst + x1 * 4;
			for (; y1 <= y2; y1++) {
//					uint32* handle = (uint32*)(offset + y1 * bpr);
//					for (int32 x = x1; x <= x2; x++) {
//						*handle++ = color.data32;
//					}
				gfxset32(offset + y1 * bpr, color.data32, (x2 - x1 + 1) * 4);
			}
		}
	} while (fBaseRenderer.next_clip_box());
}


// FillRectVerticalGradient
void
Painter::FillRectVerticalGradient(BRect r, const BGradientLinear& gradient) const
{
	if (!fValidClipping)
		return;

	// Make sure the color array is no larger than the screen height.
	r = r & fClippingRegion->Frame();

	int32 gradientArraySize = r.IntegerHeight() + 1;
	uint32 gradientArray[gradientArraySize];
	int32 gradientTop = (int32)gradient.Start().y;
	int32 gradientBottom = (int32)gradient.End().y;
	int32 colorCount = gradientBottom - gradientTop + 1;
	if (colorCount < 0) {
		// Gradient is upside down. That's currently not supported by this
		// method.
		return;
	}

	_MakeGradient(gradient, colorCount, gradientArray,
		gradientTop - (int32)r.top, gradientArraySize);

	uint8* dst = fBuffer.row_ptr(0);
	uint32 bpr = fBuffer.stride();
	int32 left = (int32)r.left;
	int32 top = (int32)r.top;
	int32 right = (int32)r.right;
	int32 bottom = (int32)r.bottom;
	// fill rects, iterate over clipping boxes
	fBaseRenderer.first_clip_box();
	do {
		int32 x1 = max_c(fBaseRenderer.xmin(), left);
		int32 x2 = min_c(fBaseRenderer.xmax(), right);
		if (x1 <= x2) {
			int32 y1 = max_c(fBaseRenderer.ymin(), top);
			int32 y2 = min_c(fBaseRenderer.ymax(), bottom);
			uint8* offset = dst + x1 * 4;
			for (; y1 <= y2; y1++) {
//					uint32* handle = (uint32*)(offset + y1 * bpr);
//					for (int32 x = x1; x <= x2; x++) {
//						*handle++ = gradientArray[y1 - top];
//					}
				gfxset32(offset + y1 * bpr, gradientArray[y1 - top],
					(x2 - x1 + 1) * 4);
			}
		}
	} while (fBaseRenderer.next_clip_box());
}


// FillRectNoClipping
void
Painter::FillRectNoClipping(const clipping_rect& r, const rgb_color& c) const
{
	int32 y = (int32)r.top;

	uint8* dst = fBuffer.row_ptr(y) + r.left * 4;
	uint32 bpr = fBuffer.stride();
	int32 bytes = (r.right - r.left + 1) * 4;

	// get a 32 bit pixel ready with the color
	pixel32 color;
	color.data8[0] = c.blue;
	color.data8[1] = c.green;
	color.data8[2] = c.red;
	color.data8[3] = c.alpha;

	for (; y <= r.bottom; y++) {
//			uint32* handle = (uint32*)dst;
//			for (int32 x = left; x <= right; x++) {
//				*handle++ = color.data32;
//			}
		gfxset32(dst, color.data32, bytes);
		dst += bpr;
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
	}

	// NOTE: This implementation might seem a little strange, but it makes
	// stroked round rects look like on R5. A more correct way would be to
	// use _StrokePath() as above (independent from fPenSize).
	// The fact that the bounding box of the round rect is not enlarged
	// by fPenSize/2 is actually on purpose, though one could argue it is
	// unexpected.

	// enclose the right and bottom edge
	rb.x++;
	rb.y++;

	agg::rounded_rect outer;
	outer.rect(lt.x, lt.y, rb.x, rb.y);
	outer.radius(xRadius, yRadius);

	if (gSubpixelAntialiasing) {
		fSubpixRasterizer.reset();
		fSubpixRasterizer.add_path(outer);

		// don't add an inner hole if the "size is negative", this avoids
		// some defects that can be observed on R5 and could be regarded
		// as a bug.
		if (2 * fPenSize < rb.x - lt.x && 2 * fPenSize < rb.y - lt.y) {
			agg::rounded_rect inner;
			inner.rect(lt.x + fPenSize, lt.y + fPenSize, rb.x - fPenSize,
				rb.y - fPenSize);
			inner.radius(max_c(0.0, xRadius - fPenSize),
				max_c(0.0, yRadius - fPenSize));

			fSubpixRasterizer.add_path(inner);
		}

		// make the inner rect work as a hole
		fSubpixRasterizer.filling_rule(agg::fill_even_odd);

		if (fPenSize > 2) {
			agg::render_scanlines(fSubpixRasterizer, fSubpixPackedScanline,
				fSubpixRenderer);
		} else {
			agg::render_scanlines(fSubpixRasterizer, fSubpixUnpackedScanline,
				fSubpixRenderer);
		}

		fSubpixRasterizer.filling_rule(agg::fill_non_zero);
	} else {
		fRasterizer.reset();
		fRasterizer.add_path(outer);

		// don't add an inner hole if the "size is negative", this avoids
		// some defects that can be observed on R5 and could be regarded as
		// a bug.
		if (2 * fPenSize < rb.x - lt.x && 2 * fPenSize < rb.y - lt.y) {
			agg::rounded_rect inner;
			inner.rect(lt.x + fPenSize, lt.y + fPenSize, rb.x - fPenSize,
				rb.y - fPenSize);
			inner.radius(max_c(0.0, xRadius - fPenSize),
				max_c(0.0, yRadius - fPenSize));

			fRasterizer.add_path(inner);
		}

		// make the inner rect work as a hole
		fRasterizer.filling_rule(agg::fill_even_odd);

		if (fPenSize > 2)
			agg::render_scanlines(fRasterizer, fPackedScanline, fRenderer);
		else
			agg::render_scanlines(fRasterizer, fUnpackedScanline, fRenderer);

		// reset to default
		fRasterizer.filling_rule(agg::fill_non_zero);
	}

	return _Clipped(_BoundingBox(outer));
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


// FillRoundRect
BRect
Painter::FillRoundRect(const BRect& r, float xRadius, float yRadius,
	const BGradient& gradient) const
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

	return _FillPath(rect, gradient);
}


// AlignEllipseRect
void
Painter::AlignEllipseRect(BRect* rect, bool filled) const
{
	if (!fSubpixelPrecise) {
		// align rect to pixels
		align_rect_to_pixels(rect);
		// account for "pixel index" versus "pixel area"
		rect->right++;
		rect->bottom++;
		if (!filled && fmodf(fPenSize, 2.0) != 0.0) {
			// align the stroke
			rect->InsetBy(0.5, 0.5);
		}
	}
}


// DrawEllipse
BRect
Painter::DrawEllipse(BRect r, bool fill) const
{
	CHECK_CLIPPING

	AlignEllipseRect(&r, fill);

	float xRadius = r.Width() / 2.0;
	float yRadius = r.Height() / 2.0;
	BPoint center(r.left + xRadius, r.top + yRadius);

	int32 divisions = (int32)((xRadius + yRadius + 2 * fPenSize) * M_PI / 2);
	if (divisions < 12)
		divisions = 12;
	if (divisions > 4096)
		divisions = 4096;

	if (fill) {
		agg::ellipse path(center.x, center.y, xRadius, yRadius, divisions);

		return _FillPath(path);
	}

	// NOTE: This implementation might seem a little strange, but it makes
	// stroked ellipses look like on R5. A more correct way would be to use
	// _StrokePath(), but it currently has its own set of problems with
	// narrow ellipses (for small xRadii or yRadii).
	float inset = fPenSize / 2.0;
	agg::ellipse inner(center.x, center.y, max_c(0.0, xRadius - inset),
		max_c(0.0, yRadius - inset), divisions);
	agg::ellipse outer(center.x, center.y, xRadius + inset, yRadius + inset,
		divisions);

	if (gSubpixelAntialiasing) {
		fSubpixRasterizer.reset();
		fSubpixRasterizer.add_path(outer);
		fSubpixRasterizer.add_path(inner);

		// make the inner ellipse work as a hole
		fSubpixRasterizer.filling_rule(agg::fill_even_odd);

		if (fPenSize > 4) {
			agg::render_scanlines(fSubpixRasterizer, fSubpixPackedScanline,
				fSubpixRenderer);
		} else {
			agg::render_scanlines(fSubpixRasterizer, fSubpixUnpackedScanline,
				fSubpixRenderer);
		}

		// reset to default
		fSubpixRasterizer.filling_rule(agg::fill_non_zero);
	} else {
		fRasterizer.reset();
		fRasterizer.add_path(outer);
		fRasterizer.add_path(inner);

		// make the inner ellipse work as a hole
		fRasterizer.filling_rule(agg::fill_even_odd);

		if (fPenSize > 4)
			agg::render_scanlines(fRasterizer, fPackedScanline, fRenderer);
		else
			agg::render_scanlines(fRasterizer, fUnpackedScanline, fRenderer);

		// reset to default
		fRasterizer.filling_rule(agg::fill_non_zero);
	}

	return _Clipped(_BoundingBox(outer));
}


// FillEllipse
BRect
Painter::FillEllipse(BRect r, const BGradient& gradient) const
{
	CHECK_CLIPPING

	AlignEllipseRect(&r, true);

	float xRadius = r.Width() / 2.0;
	float yRadius = r.Height() / 2.0;
	BPoint center(r.left + xRadius, r.top + yRadius);

	int32 divisions = (int32)((xRadius + yRadius + 2 * fPenSize) * M_PI / 2);
	if (divisions < 12)
		divisions = 12;
	if (divisions > 4096)
		divisions = 4096;

	agg::ellipse path(center.x, center.y, xRadius, yRadius, divisions);

	return _FillPath(path, gradient);
}


// StrokeArc
BRect
Painter::StrokeArc(BPoint center, float xRadius, float yRadius, float angle,
	float span) const
{
	CHECK_CLIPPING

	_Transform(&center);

	double angleRad = (angle * M_PI) / 180.0;
	double spanRad = (span * M_PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius, -angleRad,
		-spanRad);

	agg::conv_curve<agg::bezier_arc> path(arc);
	path.approximation_scale(2.0);

	return _StrokePath(path);
}


// FillArc
BRect
Painter::FillArc(BPoint center, float xRadius, float yRadius, float angle,
	float span) const
{
	CHECK_CLIPPING

	_Transform(&center);

	double angleRad = (angle * M_PI) / 180.0;
	double spanRad = (span * M_PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius, -angleRad,
		-spanRad);

	agg::conv_curve<agg::bezier_arc> segmentedArc(arc);

	fPath.remove_all();

	// build a new path by starting at the center point,
	// then traversing the arc, then going back to the center
	fPath.move_to(center.x, center.y);

	segmentedArc.rewind(0);
	double x;
	double y;
	unsigned cmd = segmentedArc.vertex(&x, &y);
	while (!agg::is_stop(cmd)) {
		fPath.line_to(x, y);
		cmd = segmentedArc.vertex(&x, &y);
	}

	fPath.close_polygon();

	return _FillPath(fPath);
}


// FillArc
BRect
Painter::FillArc(BPoint center, float xRadius, float yRadius, float angle,
	float span, const BGradient& gradient) const
{
	CHECK_CLIPPING

	_Transform(&center);

	double angleRad = (angle * M_PI) / 180.0;
	double spanRad = (span * M_PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius, -angleRad,
		-spanRad);

	agg::conv_curve<agg::bezier_arc> segmentedArc(arc);

	fPath.remove_all();

	// build a new path by starting at the center point,
	// then traversing the arc, then going back to the center
	fPath.move_to(center.x, center.y);

	segmentedArc.rewind(0);
	double x;
	double y;
	unsigned cmd = segmentedArc.vertex(&x, &y);
	while (!agg::is_stop(cmd)) {
		fPath.line_to(x, y);
		cmd = segmentedArc.vertex(&x, &y);
	}

	fPath.close_polygon();

	return _FillPath(fPath, gradient);
}


// #pragma mark -


// DrawString
BRect
Painter::DrawString(const char* utf8String, uint32 length, BPoint baseLine,
	const escapement_delta* delta, FontCacheReference* cacheReference)
{
	CHECK_CLIPPING

	if (!fSubpixelPrecise) {
		baseLine.x = roundf(baseLine.x);
		baseLine.y = roundf(baseLine.y);
	}

	BRect bounds;

	// text is not rendered with patterns, but we need to
	// make sure that the previous pattern is restored
	pattern oldPattern = *fPatternHandler.GetR5Pattern();
	SetPattern(B_SOLID_HIGH, true);

	bounds = fTextRenderer.RenderString(utf8String, length,
		baseLine, fClippingRegion->Frame(), false, NULL, delta,
		cacheReference);

	SetPattern(oldPattern);

	return _Clipped(bounds);
}


// DrawString
BRect
Painter::DrawString(const char* utf8String, uint32 length,
	const BPoint* offsets, FontCacheReference* cacheReference)
{
	CHECK_CLIPPING

	// TODO: Round offsets to device pixel grid if !fSubpixelPrecise?

	BRect bounds;

	// text is not rendered with patterns, but we need to
	// make sure that the previous pattern is restored
	pattern oldPattern = *fPatternHandler.GetR5Pattern();
	SetPattern(B_SOLID_HIGH, true);

	bounds = fTextRenderer.RenderString(utf8String, length,
		offsets, fClippingRegion->Frame(), false, NULL,
		cacheReference);

	SetPattern(oldPattern);

	return _Clipped(bounds);
}


// BoundingBox
BRect
Painter::BoundingBox(const char* utf8String, uint32 length, BPoint baseLine,
	BPoint* penLocation, const escapement_delta* delta,
	FontCacheReference* cacheReference) const
{
	if (!fSubpixelPrecise) {
		baseLine.x = roundf(baseLine.x);
		baseLine.y = roundf(baseLine.y);
	}

	static BRect dummy;
	return fTextRenderer.RenderString(utf8String, length,
		baseLine, dummy, true, penLocation, delta, cacheReference);
}


// BoundingBox
BRect
Painter::BoundingBox(const char* utf8String, uint32 length,
	const BPoint* offsets, BPoint* penLocation,
	FontCacheReference* cacheReference) const
{
	// TODO: Round offsets to device pixel grid if !fSubpixelPrecise?

	static BRect dummy;
	return fTextRenderer.RenderString(utf8String, length,
		offsets, dummy, true, penLocation, cacheReference);
}


// StringWidth
float
Painter::StringWidth(const char* utf8String, uint32 length,
	const escapement_delta* delta)
{
	return Font().StringWidth(utf8String, length, delta);
}


// #pragma mark -


// DrawBitmap
BRect
Painter::DrawBitmap(const ServerBitmap* bitmap, BRect bitmapRect,
	BRect viewRect, uint32 options) const
{
	CHECK_CLIPPING

	BRect touched = _Clipped(viewRect);

	if (bitmap && bitmap->IsValid() && touched.IsValid()) {
		// the native bitmap coordinate system
		BRect actualBitmapRect(bitmap->Bounds());

		TRACE("Painter::DrawBitmap()\n");
		TRACE("   actualBitmapRect = (%.1f, %.1f) - (%.1f, %.1f)\n",
			actualBitmapRect.left, actualBitmapRect.top,
			actualBitmapRect.right, actualBitmapRect.bottom);
		TRACE("   bitmapRect = (%.1f, %.1f) - (%.1f, %.1f)\n",
			bitmapRect.left, bitmapRect.top, bitmapRect.right,
			bitmapRect.bottom);
		TRACE("   viewRect = (%.1f, %.1f) - (%.1f, %.1f)\n",
			viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);

		agg::rendering_buffer srcBuffer;
		srcBuffer.attach(bitmap->Bits(), bitmap->Width(), bitmap->Height(),
			bitmap->BytesPerRow());

		_DrawBitmap(srcBuffer, bitmap->ColorSpace(), actualBitmapRect,
			bitmapRect, viewRect, options);
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


// FillRegion
BRect
Painter::FillRegion(const BRegion* region, const BGradient& gradient) const
{
	CHECK_CLIPPING

	BRegion copy(*region);
	int32 count = copy.CountRects();
	BRect touched = FillRect(copy.RectAt(0), gradient);
	for (int32 i = 1; i < count; i++) {
		touched = touched | FillRect(copy.RectAt(i), gradient);
	}
	return touched;
}


// InvertRect
BRect
Painter::InvertRect(const BRect& r) const
{
	CHECK_CLIPPING

	BRegion region(r);
	region.IntersectWith(fClippingRegion);

	// implementation only for B_RGB32 at the moment
	int32 count = region.CountRects();
	for (int32 i = 0; i < count; i++)
		_InvertRect32(region.RectAt(i));

	return _Clipped(r);
}


// #pragma mark - private


// _Transform
inline void
Painter::_Transform(BPoint* point, bool centerOffset) const
{
	// rounding
	if (!fSubpixelPrecise) {
		// TODO: validate usage of floor() for values < 0
		point->x = (int32)point->x;
		point->y = (int32)point->y;
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
inline BPoint
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
	if (rect.IsValid())
		return BRect(rect & fClippingRegion->Frame());

	return BRect(rect);
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
	// The last parameter to SetDrawingMode() is a special flag
	// for when Painter is used to draw text. In this case, another
	// special version of B_OP_COPY is used that acts like R5 in that
	// anti-aliased pixel are not rendered against the actual background
	// but the current low color instead. This way, the frame buffer
	// doesn't need to be read.
	// When a solid pattern is used, _SetRendererColor()
	// has to be called so that all internal colors in the renderes
	// are up to date for use by the solid drawing mode version.
	fPixelFormat.SetDrawingMode(fDrawingMode, fAlphaSrcMode, fAlphaFncMode,
		drawingText);
	if (drawingText)
		fPatternHandler.MakeOpCopyColorCache();
}


// _SetRendererColor
void
Painter::_SetRendererColor(const rgb_color& color) const
{
	fRenderer.color(agg::rgba(color.red / 255.0, color.green / 255.0,
		color.blue / 255.0, color.alpha / 255.0));
	fSubpixRenderer.color(agg::rgba(color.red / 255.0, color.green / 255.0,
		color.blue / 255.0, color.alpha / 255.0));
// TODO: bitmap fonts not yet correctly setup in AGGTextRenderer
//	fRendererBin.color(agg::rgba(color.red / 255.0, color.green / 255.0,
//		color.blue / 255.0, color.alpha / 255.0));
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

	fPath.remove_all();

	fPath.move_to(pt1.x, pt1.y);
	fPath.line_to(pt2.x, pt2.y);
	fPath.line_to(pt3.x, pt3.y);

	fPath.close_polygon();

	if (fill)
		return _FillPath(fPath);

	return _StrokePath(fPath);
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


// copy_bitmap_row_bgr32_over
static inline void
copy_bitmap_row_bgr32_over(uint8* dst, const uint8* src, int32 numPixels,
	const rgb_color* colorMap)
{
	uint32* d = (uint32*)dst;
	uint32* s = (uint32*)src;
	while (numPixels--) {
		if (*s != B_TRANSPARENT_MAGIC_RGBA32)
			*(uint32*)d = *(uint32*)s;
		d++;
		s++;
	}
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
		d++;
		b += 4;
		src += 4;
	}
	memcpy(dst, buffer, bytes);
}


// _TransparentMagicToAlpha
template<typename sourcePixel>
void
Painter::_TransparentMagicToAlpha(sourcePixel* buffer, uint32 width,
	uint32 height, uint32 sourceBytesPerRow, sourcePixel transparentMagic,
	BBitmap* output) const
{
	uint8* sourceRow = (uint8*)buffer;
	uint8* destRow = (uint8*)output->Bits();
	uint32 destBytesPerRow = output->BytesPerRow();

	for (uint32 y = 0; y < height; y++) {
		sourcePixel* pixel = (sourcePixel*)sourceRow;
		uint32* destPixel = (uint32*)destRow;
		for (uint32 x = 0; x < width; x++, pixel++, destPixel++) {
			if (*pixel == transparentMagic)
				*destPixel &= 0x00ffffff;
		}

		sourceRow += sourceBytesPerRow;
		destRow += destBytesPerRow;
	}
}


// _DrawBitmap
void
Painter::_DrawBitmap(agg::rendering_buffer& srcBuffer, color_space format,
	BRect actualBitmapRect, BRect bitmapRect, BRect viewRect,
	uint32 options) const
{
	if (!fValidClipping
		|| !bitmapRect.IsValid() || !bitmapRect.Intersects(actualBitmapRect)
		|| !viewRect.IsValid()) {
		return;
	}

	if (!fSubpixelPrecise) {
		align_rect_to_pixels(&bitmapRect);
		align_rect_to_pixels(&viewRect);
	}

	TRACE("Painter::_DrawBitmap()\n");
	TRACE("   bitmapRect = (%.1f, %.1f) - (%.1f, %.1f)\n",
		bitmapRect.left, bitmapRect.top, bitmapRect.right, bitmapRect.bottom);
	TRACE("   viewRect = (%.1f, %.1f) - (%.1f, %.1f)\n",
		viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);

	double xScale = (viewRect.Width() + 1) / (bitmapRect.Width() + 1);
	double yScale = (viewRect.Height() + 1) / (bitmapRect.Height() + 1);

	if (xScale == 0.0 || yScale == 0.0)
		return;

	// compensate for the lefttop offset the actualBitmapRect might have
	// actualBitmapRect has the right size, but put it at B_ORIGIN
	// bitmapRect is already in good coordinates
	actualBitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);

	// constrain rect to passed bitmap bounds
	// and transfer the changes to the viewRect with the right scale
	if (bitmapRect.left < actualBitmapRect.left) {
		float diff = actualBitmapRect.left - bitmapRect.left;
		viewRect.left += diff * xScale;
		bitmapRect.left = actualBitmapRect.left;
	}
	if (bitmapRect.top < actualBitmapRect.top) {
		float diff = actualBitmapRect.top - bitmapRect.top;
		viewRect.top += diff * yScale;
		bitmapRect.top = actualBitmapRect.top;
	}
	if (bitmapRect.right > actualBitmapRect.right) {
		float diff = bitmapRect.right - actualBitmapRect.right;
		viewRect.right -= diff * xScale;
		bitmapRect.right = actualBitmapRect.right;
	}
	if (bitmapRect.bottom > actualBitmapRect.bottom) {
		float diff = bitmapRect.bottom - actualBitmapRect.bottom;
		viewRect.bottom -= diff * yScale;
		bitmapRect.bottom = actualBitmapRect.bottom;
	}

	double xOffset = viewRect.left - bitmapRect.left;
	double yOffset = viewRect.top - bitmapRect.top;

	// optimized code path for B_CMAP8 and no scale
	if (xScale == 1.0 && yScale == 1.0) {
		if (format == B_CMAP8) {
			if (fDrawingMode == B_OP_COPY) {
				_DrawBitmapNoScale32(copy_bitmap_row_cmap8_copy, 1,
					srcBuffer, (int32)xOffset, (int32)yOffset, viewRect);
				return;
			}
			if (fDrawingMode == B_OP_OVER) {
				_DrawBitmapNoScale32(copy_bitmap_row_cmap8_over, 1,
					srcBuffer, (int32)xOffset, (int32)yOffset, viewRect);
				return;
			}
		} else if (format == B_RGB32) {
			if (fDrawingMode == B_OP_OVER) {
				_DrawBitmapNoScale32(copy_bitmap_row_bgr32_over, 4,
					srcBuffer, (int32)xOffset, (int32)yOffset, viewRect);
				return;
			}
		}
	}

	BBitmap* temp = NULL;
	ObjectDeleter<BBitmap> tempDeleter;

	if ((format != B_RGBA32 && format != B_RGB32)
		|| (format == B_RGB32 && fDrawingMode != B_OP_COPY
#if 1
// Enabling this would make the behavior compatible to BeOS, which
// treats B_RGB32 bitmaps as B_RGB*A*32 bitmaps in B_OP_ALPHA - unlike in
// all other drawing modes, where B_TRANSPARENT_MAGIC_RGBA32 is handled.
// B_RGB32 bitmaps therefore don't draw correctly on BeOS if they actually
// use this color, unless the alpha channel contains 255 for all other
// pixels, which is inconsistent.
			&& fDrawingMode != B_OP_ALPHA
#endif
		)) {
		temp = new (nothrow) BBitmap(actualBitmapRect, B_BITMAP_NO_SERVER_LINK,
			B_RGBA32);
		if (temp == NULL) {
			fprintf(stderr, "Painter::_DrawBitmap() - "
				"out of memory for creating temporary conversion bitmap\n");
			return;
		}

		tempDeleter.SetTo(temp);

		status_t err = temp->ImportBits(srcBuffer.buf(),
			srcBuffer.height() * srcBuffer.stride(),
			srcBuffer.stride(), 0, format);
		if (err < B_OK) {
			fprintf(stderr, "Painter::_DrawBitmap() - "
				"colorspace conversion failed: %s\n", strerror(err));
			return;
		}

		// the original bitmap might have had some of the
		// transaparent magic colors set that we now need to
		// make transparent in our RGBA32 bitmap again.
		switch (format) {
			case B_RGB32:
				_TransparentMagicToAlpha((uint32 *)srcBuffer.buf(),
					srcBuffer.width(), srcBuffer.height(),
					srcBuffer.stride(), B_TRANSPARENT_MAGIC_RGBA32,
					temp);
				break;

			// TODO: not sure if this applies to B_RGBA15 too. It
			// should not because B_RGBA15 actually has an alpha
			// channel itself and it should have been preserved
			// when importing the bitmap. Maybe it applies to
			// B_RGB16 though?
			case B_RGB15:
				_TransparentMagicToAlpha((uint16 *)srcBuffer.buf(),
					srcBuffer.width(), srcBuffer.height(),
					srcBuffer.stride(), B_TRANSPARENT_MAGIC_RGBA15,
					temp);
				break;

			default:
				break;
		}

		srcBuffer.attach((uint8*)temp->Bits(),
			(uint32)actualBitmapRect.IntegerWidth() + 1,
			(uint32)actualBitmapRect.IntegerHeight() + 1,
			temp->BytesPerRow());
	}

	// maybe we can use an optimized version if there is no scale
	if (xScale == 1.0 && yScale == 1.0) {
		if (fDrawingMode == B_OP_COPY) {
			_DrawBitmapNoScale32(copy_bitmap_row_bgr32_copy, 4, srcBuffer,
				(int32)xOffset, (int32)yOffset, viewRect);
			return;
		}
		if (fDrawingMode == B_OP_OVER || (fDrawingMode == B_OP_ALPHA
				 && fAlphaSrcMode == B_PIXEL_ALPHA
				 && fAlphaFncMode == B_ALPHA_OVERLAY)) {
			_DrawBitmapNoScale32(copy_bitmap_row_bgr32_alpha, 4, srcBuffer,
				(int32)xOffset, (int32)yOffset, viewRect);
			return;
		}
	}

	if (fDrawingMode == B_OP_COPY) {
		if ((options & B_FILTER_BITMAP_BILINEAR) != 0) {
			_DrawBitmapBilinearCopy32(srcBuffer, xOffset, yOffset, xScale,
				yScale, viewRect);
		} else {
			_DrawBitmapNearestNeighborCopy32(srcBuffer, xOffset, yOffset,
				xScale, yScale, viewRect);
		}
		return;
	}

	// for all other cases (non-optimized drawing mode or scaled drawing)
	_DrawBitmapGeneric32(srcBuffer, xOffset, yOffset, xScale, yScale, viewRect,
		options);
}


#define DEBUG_DRAW_BITMAP 0


// _DrawBitmapNoScale32
template <class F>
void
Painter::_DrawBitmapNoScale32(F copyRowFunction, uint32 bytesPerSourcePixel,
	agg::rendering_buffer& srcBuffer, int32 xOffset, int32 yOffset,
	BRect viewRect) const
{
	// NOTE: this would crash if viewRect was large enough to read outside the
	// bitmap, so make sure this is not the case before calling this function!
	uint8* dst = fBuffer.row_ptr(0);
	uint32 dstBPR = fBuffer.stride();

	const uint8* src = srcBuffer.row_ptr(0);
	uint32 srcBPR = srcBuffer.stride();

	int32 left = (int32)viewRect.left;
	int32 top = (int32)viewRect.top;
	int32 right = (int32)viewRect.right;
	int32 bottom = (int32)viewRect.bottom;

#if DEBUG_DRAW_BITMAP
if (left - xOffset < 0 || left - xOffset >= (int32)srcBuffer.width() ||
	right - xOffset >= (int32)srcBuffer.width() ||
	top - yOffset < 0 || top - yOffset >= (int32)srcBuffer.height() ||
	bottom - yOffset >= (int32)srcBuffer.height()) {

	char message[256];
	sprintf(message, "reading outside of bitmap (%ld, %ld, %ld, %ld) "
			"(%d, %d) (%ld, %ld)",
		left - xOffset, top - yOffset, right - xOffset, bottom - yOffset,
		srcBuffer.width(), srcBuffer.height(), xOffset, yOffset);
	debugger(message);
}
#endif

	const rgb_color* colorMap = SystemPalette();

	// copy rects, iterate over clipping boxes
	fBaseRenderer.first_clip_box();
	do {
		int32 x1 = max_c(fBaseRenderer.xmin(), left);
		int32 x2 = min_c(fBaseRenderer.xmax(), right);
		if (x1 <= x2) {
			int32 y1 = max_c(fBaseRenderer.ymin(), top);
			int32 y2 = min_c(fBaseRenderer.ymax(), bottom);
			if (y1 <= y2) {
				uint8* dstHandle = dst + y1 * dstBPR + x1 * 4;
				const uint8* srcHandle = src + (y1 - yOffset) * srcBPR
					+ (x1 - xOffset) * bytesPerSourcePixel;

				for (; y1 <= y2; y1++) {
					copyRowFunction(dstHandle, srcHandle, x2 - x1 + 1, colorMap);

					dstHandle += dstBPR;
					srcHandle += srcBPR;
				}
			}
		}
	} while (fBaseRenderer.next_clip_box());
}


// _DrawBitmapNearestNeighborCopy32
void
Painter::_DrawBitmapNearestNeighborCopy32(agg::rendering_buffer& srcBuffer,
	double xOffset, double yOffset, double xScale, double yScale,
	BRect viewRect) const
{
	//bigtime_t now = system_time();
	uint32 dstWidth = viewRect.IntegerWidth() + 1;
	uint32 dstHeight = viewRect.IntegerHeight() + 1;
	uint32 srcWidth = srcBuffer.width();
	uint32 srcHeight = srcBuffer.height();

	// Do not calculate more filter weights than necessary and also
	// keep the stack based allocations reasonably sized
	if (fClippingRegion->Frame().IntegerWidth() + 1 < (int32)dstWidth)
		dstWidth = fClippingRegion->Frame().IntegerWidth() + 1;
	if (fClippingRegion->Frame().IntegerHeight() + 1 < (int32)dstHeight)
		dstHeight = fClippingRegion->Frame().IntegerHeight() + 1;

	// When calculating less filter weights than specified by viewRect,
	// we need to compensate the offset.
	uint32 filterWeightXIndexOffset = 0;
	uint32 filterWeightYIndexOffset = 0;
	if (fClippingRegion->Frame().left > viewRect.left) {
		filterWeightXIndexOffset = (int32)(fClippingRegion->Frame().left
			- viewRect.left);
	}
	if (fClippingRegion->Frame().top > viewRect.top) {
		filterWeightYIndexOffset = (int32)(fClippingRegion->Frame().top
			- viewRect.top);
	}

	// should not pose a problem with stack overflows
	// (needs around 6Kb for 1920x1200)
	uint16 xIndices[dstWidth];
	uint16 yIndices[dstHeight];

	// Extract the cropping information for the source bitmap,
	// If only a part of the source bitmap is to be drawn with scale,
	// the offset will be different from the viewRect left top corner.
	int32 xBitmapShift = (int32)(viewRect.left - xOffset);
	int32 yBitmapShift = (int32)(viewRect.top - yOffset);

	for (uint32 i = 0; i < dstWidth; i++) {
		// index into source
		uint16 index = (uint16)((i + filterWeightXIndexOffset) * srcWidth
			/ (srcWidth * xScale));
		// round down to get the left pixel
		xIndices[i] = index;
		// handle cropped source bitmap
		xIndices[i] += xBitmapShift;
		// precompute index for 32 bit pixels
		xIndices[i] *= 4;
	}

	for (uint32 i = 0; i < dstHeight; i++) {
		// index into source
		uint16 index = (uint16)((i + filterWeightYIndexOffset) * srcHeight
			/ (srcHeight * yScale));
		// round down to get the top pixel
		yIndices[i] = index;
		// handle cropped source bitmap
		yIndices[i] += yBitmapShift;
	}
//printf("X: %d ... %d, %d (%ld or %f)\n",
//	xIndices[0], xIndices[dstWidth - 2], xIndices[dstWidth - 1], dstWidth,
//	srcWidth * xScale);
//printf("Y: %d ... %d, %d (%ld or %f)\n",
//	yIndices[0], yIndices[dstHeight - 2], yIndices[dstHeight - 1], dstHeight,
//	srcHeight * yScale);

	const int32 left = (int32)viewRect.left;
	const int32 top = (int32)viewRect.top;
	const int32 right = (int32)viewRect.right;
	const int32 bottom = (int32)viewRect.bottom;

	const uint32 dstBPR = fBuffer.stride();

	// iterate over clipping boxes
	fBaseRenderer.first_clip_box();
	do {
		const int32 x1 = max_c(fBaseRenderer.xmin(), left);
		const int32 x2 = min_c(fBaseRenderer.xmax(), right);
		if (x1 > x2)
			continue;

		int32 y1 = max_c(fBaseRenderer.ymin(), top);
		int32 y2 = min_c(fBaseRenderer.ymax(), bottom);
		if (y1 > y2)
			continue;

		// buffer offset into destination
		uint8* dst = fBuffer.row_ptr(y1) + x1 * 4;

		// x and y are needed as indeces into the wheight arrays, so the
		// offset into the target buffer needs to be compensated
		const int32 xIndexL = x1 - left - filterWeightXIndexOffset;
		const int32 xIndexR = x2 - left - filterWeightXIndexOffset;
		y1 -= top + filterWeightYIndexOffset;
		y2 -= top + filterWeightYIndexOffset;

//printf("x: %ld - %ld\n", xIndexL, xIndexR);
//printf("y: %ld - %ld\n", y1, y2);

		for (; y1 <= y2; y1++) {
			// buffer offset into source (top row)
			register const uint8* src = srcBuffer.row_ptr(yIndices[y1]);
			// buffer handle for destination to be incremented per pixel
			register uint32* d = (uint32*)dst;

			for (int32 x = xIndexL; x <= xIndexR; x++) {
				*d = *(uint32*)(src + xIndices[x]);
				d++;
			}
			dst += dstBPR;
		}
	} while (fBaseRenderer.next_clip_box());

//printf("draw bitmap %.5fx%.5f: %lld\n", xScale, yScale, system_time() - now);
}


// _DrawBitmapBilinearCopy32
void
Painter::_DrawBitmapBilinearCopy32(agg::rendering_buffer& srcBuffer,
	double xOffset, double yOffset, double xScale, double yScale,
	BRect viewRect) const
{
	//bigtime_t now = system_time();
	uint32 dstWidth = viewRect.IntegerWidth() + 1;
	uint32 dstHeight = viewRect.IntegerHeight() + 1;
	uint32 srcWidth = srcBuffer.width();
	uint32 srcHeight = srcBuffer.height();

	// Do not calculate more filter weights than necessary and also
	// keep the stack based allocations reasonably sized
	if (fClippingRegion->Frame().IntegerWidth() + 1 < (int32)dstWidth)
		dstWidth = fClippingRegion->Frame().IntegerWidth() + 1;
	if (fClippingRegion->Frame().IntegerHeight() + 1 < (int32)dstHeight)
		dstHeight = fClippingRegion->Frame().IntegerHeight() + 1;

	// When calculating less filter weights than specified by viewRect,
	// we need to compensate the offset.
	uint32 filterWeightXIndexOffset = 0;
	uint32 filterWeightYIndexOffset = 0;
	if (fClippingRegion->Frame().left > viewRect.left) {
		filterWeightXIndexOffset = (int32)(fClippingRegion->Frame().left
			- viewRect.left);
	}
	if (fClippingRegion->Frame().top > viewRect.top) {
		filterWeightYIndexOffset = (int32)(fClippingRegion->Frame().top
			- viewRect.top);
	}

	struct FilterInfo {
		uint16 index;	// index into source bitmap row/column
		uint16 weight;	// weight of the pixel at index [0..255]
	};

//#define FILTER_INFOS_ON_HEAP
#ifdef FILTER_INFOS_ON_HEAP
	FilterInfo* xWeights = new (nothrow) FilterInfo[dstWidth];
	FilterInfo* yWeights = new (nothrow) FilterInfo[dstHeight];
	if (xWeights == NULL || yWeights == NULL) {
		delete[] xWeights;
		delete[] yWeights;
		return;
	}
#else
	// stack based saves about 200µs on 1.85 GHz Core 2 Duo
	// should not pose a problem with stack overflows
	// (needs around 12Kb for 1920x1200)
	FilterInfo xWeights[dstWidth];
	FilterInfo yWeights[dstHeight];
#endif

	// Extract the cropping information for the source bitmap,
	// If only a part of the source bitmap is to be drawn with scale,
	// the offset will be different from the viewRect left top corner.
	int32 xBitmapShift = (int32)(viewRect.left - xOffset);
	int32 yBitmapShift = (int32)(viewRect.top - yOffset);

	for (uint32 i = 0; i < dstWidth; i++) {
		// fractional index into source
		// NOTE: It is very important to calculate the fractional index
		// into the source pixel grid like this to prevent out of bounds
		// access! It will result in the rightmost pixel of the destination
		// to access the rightmost pixel of the source with a weighting
		// of 255. This in turn will trigger an optimization in the loop
		// that also prevents out of bounds access.
		float index = (i + filterWeightXIndexOffset) * (srcWidth - 1)
			/ (srcWidth * xScale - 1);
		// round down to get the left pixel
		xWeights[i].index = (uint16)index;
		xWeights[i].weight = 255 - (uint16)((index - xWeights[i].index) * 255);
		// handle cropped source bitmap
		xWeights[i].index += xBitmapShift;
		// precompute index for 32 bit pixels
		xWeights[i].index *= 4;
	}

	for (uint32 i = 0; i < dstHeight; i++) {
		// fractional index into source
		// NOTE: It is very important to calculate the fractional index
		// into the source pixel grid like this to prevent out of bounds
		// access! It will result in the bottommost pixel of the destination
		// to access the bottommost pixel of the source with a weighting
		// of 255. This in turn will trigger an optimization in the loop
		// that also prevents out of bounds access.
		float index = (i + filterWeightYIndexOffset) * (srcHeight - 1)
			/ (srcHeight * yScale - 1);
		// round down to get the top pixel
		yWeights[i].index = (uint16)index;
		yWeights[i].weight = 255 - (uint16)((index - yWeights[i].index) * 255);
		// handle cropped source bitmap
		yWeights[i].index += yBitmapShift;
	}
//printf("X: %d/%d ... %d/%d, %d/%d (%ld)\n",
//	xWeights[0].index, xWeights[0].weight,
//	xWeights[dstWidth - 2].index, xWeights[dstWidth - 2].weight,
//	xWeights[dstWidth - 1].index, xWeights[dstWidth - 1].weight,
//	dstWidth);
//printf("Y: %d/%d ... %d/%d, %d/%d (%ld)\n",
//	yWeights[0].index, yWeights[0].weight,
//	yWeights[dstHeight - 2].index, yWeights[dstHeight - 2].weight,
//	yWeights[dstHeight - 1].index, yWeights[dstHeight - 1].weight,
//	dstHeight);

	const int32 left = (int32)viewRect.left;
	const int32 top = (int32)viewRect.top;
	const int32 right = (int32)viewRect.right;
	const int32 bottom = (int32)viewRect.bottom;

	const uint32 dstBPR = fBuffer.stride();
	const uint32 srcBPR = srcBuffer.stride();

	// Figure out which version of the code we want to use...
	enum {
		kOptimizeForLowFilterRatio = 0,
		kUseDefaultVersion,
		kUseSIMDVersion
	};

	int codeSelect = kUseDefaultVersion;

	uint32 neededSIMDFlags = APPSERVER_SIMD_MMX | APPSERVER_SIMD_SSE;
	if ((sSIMDFlags & neededSIMDFlags) == neededSIMDFlags)
		codeSelect = kUseSIMDVersion;
	else {
		if (xScale == yScale && (xScale == 1.5 || xScale == 2.0
			|| xScale == 2.5 || xScale == 3.0)) {
			codeSelect = kOptimizeForLowFilterRatio;
		}
	}

	// iterate over clipping boxes
	fBaseRenderer.first_clip_box();
	do {
		const int32 x1 = max_c(fBaseRenderer.xmin(), left);
		const int32 x2 = min_c(fBaseRenderer.xmax(), right);
		if (x1 > x2)
			continue;

		int32 y1 = max_c(fBaseRenderer.ymin(), top);
		int32 y2 = min_c(fBaseRenderer.ymax(), bottom);
		if (y1 > y2)
			continue;

		// buffer offset into destination
		uint8* dst = fBuffer.row_ptr(y1) + x1 * 4;

		// x and y are needed as indeces into the wheight arrays, so the
		// offset into the target buffer needs to be compensated
		const int32 xIndexL = x1 - left - filterWeightXIndexOffset;
		const int32 xIndexR = x2 - left - filterWeightXIndexOffset;
		y1 -= top + filterWeightYIndexOffset;
		y2 -= top + filterWeightYIndexOffset;

//printf("x: %ld - %ld\n", xIndexL, xIndexR);
//printf("y: %ld - %ld\n", y1, y2);

		switch (codeSelect) {
			case kOptimizeForLowFilterRatio:
			{
				// In this mode, we anticipate to hit many destination pixels
				// that map directly to a source pixel, we have more branches
				// in the inner loop but save time because of the special
				// cases. If there are too few direct hit pixels, the branches
				// only waste time.
				for (; y1 <= y2; y1++) {
					// cache the weight of the top and bottom row
					const uint16 wTop = yWeights[y1].weight;
					const uint16 wBottom = 255 - yWeights[y1].weight;

					// buffer offset into source (top row)
					register const uint8* src
						= srcBuffer.row_ptr(yWeights[y1].index);
					// buffer handle for destination to be incremented per
					// pixel
					register uint8* d = dst;

					if (wTop == 255) {
						for (int32 x = xIndexL; x <= xIndexR; x++) {
							const uint8* s = src + xWeights[x].index;
							// This case is important to prevent out
							// of bounds access at bottom edge of the source
							// bitmap. If the scale is low and integer, it will
							// also help the speed.
							if (xWeights[x].weight == 255) {
								// As above, but to prevent out of bounds
								// on the right edge.
								*(uint32*)d = *(uint32*)s;
							} else {
								// Only the left and right pixels are
								// interpolated, since the top row has 100%
								// weight.
								const uint16 wLeft = xWeights[x].weight;
								const uint16 wRight = 255 - wLeft;
								d[0] = (s[0] * wLeft + s[4] * wRight) >> 8;
								d[1] = (s[1] * wLeft + s[5] * wRight) >> 8;
								d[2] = (s[2] * wLeft + s[6] * wRight) >> 8;
							}
							d += 4;
						}
					} else {
						for (int32 x = xIndexL; x <= xIndexR; x++) {
							const uint8* s = src + xWeights[x].index;
							if (xWeights[x].weight == 255) {
								// Prevent out of bounds access on the right
								// edge or simply speed up.
								const uint8* sBottom = s + srcBPR;
								d[0] = (s[0] * wTop + sBottom[0] * wBottom)
									>> 8;
								d[1] = (s[1] * wTop + sBottom[1] * wBottom)
									>> 8;
								d[2] = (s[2] * wTop + sBottom[2] * wBottom)
									>> 8;
							} else {
								// calculate the weighted sum of all four
								// interpolated pixels
								const uint16 wLeft = xWeights[x].weight;
								const uint16 wRight = 255 - wLeft;
								// left and right of top row
								uint32 t0 = (s[0] * wLeft + s[4] * wRight)
									* wTop;
								uint32 t1 = (s[1] * wLeft + s[5] * wRight)
									* wTop;
								uint32 t2 = (s[2] * wLeft + s[6] * wRight)
									* wTop;

								// left and right of bottom row
								s += srcBPR;
								t0 += (s[0] * wLeft + s[4] * wRight) * wBottom;
								t1 += (s[1] * wLeft + s[5] * wRight) * wBottom;
								t2 += (s[2] * wLeft + s[6] * wRight) * wBottom;

								d[0] = t0 >> 16;
								d[1] = t1 >> 16;
								d[2] = t2 >> 16;
							}
							d += 4;
						}
					}
					dst += dstBPR;
				}
				break;
			}

			case kUseDefaultVersion:
			{
				// In this mode we anticipate many pixels wich need filtering,
				// there are no special cases for direct hit pixels except for
				// the last column/row and the right/bottom corner pixel.

				// The last column/row handling does not need to be performed
				// for all clipping rects!
				int32 yMax = y2;
				if (yWeights[yMax].weight == 255)
					yMax--;
				int32 xIndexMax = xIndexR;
				if (xWeights[xIndexMax].weight == 255)
					xIndexMax--;

				for (; y1 <= yMax; y1++) {
					// cache the weight of the top and bottom row
					const uint16 wTop = yWeights[y1].weight;
					const uint16 wBottom = 255 - yWeights[y1].weight;

					// buffer offset into source (top row)
					register const uint8* src
						= srcBuffer.row_ptr(yWeights[y1].index);
					// buffer handle for destination to be incremented per
					// pixel
					register uint8* d = dst;

					for (int32 x = xIndexL; x <= xIndexMax; x++) {
						const uint8* s = src + xWeights[x].index;
						// calculate the weighted sum of all four
						// interpolated pixels
						const uint16 wLeft = xWeights[x].weight;
						const uint16 wRight = 255 - wLeft;
						// left and right of top row
						uint32 t0 = (s[0] * wLeft + s[4] * wRight) * wTop;
						uint32 t1 = (s[1] * wLeft + s[5] * wRight) * wTop;
						uint32 t2 = (s[2] * wLeft + s[6] * wRight) * wTop;

						// left and right of bottom row
						s += srcBPR;
						t0 += (s[0] * wLeft + s[4] * wRight) * wBottom;
						t1 += (s[1] * wLeft + s[5] * wRight) * wBottom;
						t2 += (s[2] * wLeft + s[6] * wRight) * wBottom;
						d[0] = t0 >> 16;
						d[1] = t1 >> 16;
						d[2] = t2 >> 16;
						d += 4;
					}
					// last column of pixels if necessary
					if (xIndexMax < xIndexR) {
						const uint8* s = src + xWeights[xIndexR].index;
						const uint8* sBottom = s + srcBPR;
						d[0] = (s[0] * wTop + sBottom[0] * wBottom) >> 8;
						d[1] = (s[1] * wTop + sBottom[1] * wBottom) >> 8;
						d[2] = (s[2] * wTop + sBottom[2] * wBottom) >> 8;
					}

					dst += dstBPR;
				}

				// last row of pixels if necessary
				// buffer offset into source (bottom row)
				register const uint8* src
					= srcBuffer.row_ptr(yWeights[y2].index);
				// buffer handle for destination to be incremented per pixel
				register uint8* d = dst;

				if (yMax < y2) {
					for (int32 x = xIndexL; x <= xIndexMax; x++) {
						const uint8* s = src + xWeights[x].index;
						const uint16 wLeft = xWeights[x].weight;
						const uint16 wRight = 255 - wLeft;
						d[0] = (s[0] * wLeft + s[4] * wRight) >> 8;
						d[1] = (s[1] * wLeft + s[5] * wRight) >> 8;
						d[2] = (s[2] * wLeft + s[6] * wRight) >> 8;
						d += 4;
					}
				}

				// pixel in bottom right corner if necessary
				if (yMax < y2 && xIndexMax < xIndexR) {
					const uint8* s = src + xWeights[xIndexR].index;
					*(uint32*)d = *(uint32*)s;
				}
				break;
			}

#ifdef __INTEL__
			case kUseSIMDVersion:
			{
				// Basically the same as the "standard" mode, but we use SIMD
				// routines for the processing of the single display lines.

				// The last column/row handling does not need to be performed
				// for all clipping rects!
				int32 yMax = y2;
				if (yWeights[yMax].weight == 255)
					yMax--;
				int32 xIndexMax = xIndexR;
				if (xWeights[xIndexMax].weight == 255)
					xIndexMax--;

				for (; y1 <= yMax; y1++) {
					// cache the weight of the top and bottom row
					const uint16 wTop = yWeights[y1].weight;
					const uint16 wBottom = 255 - yWeights[y1].weight;

					// buffer offset into source (top row)
					const uint8* src = srcBuffer.row_ptr(yWeights[y1].index);
					// buffer handle for destination to be incremented per
					// pixel
					uint8* d = dst;
					bilinear_scale_xloop_mmxsse(src, dst, xWeights,	xIndexL,
						xIndexMax, wTop, srcBPR);
					// increase pointer by processed pixels
					d += (xIndexMax - xIndexL + 1) * 4;

					// last column of pixels if necessary
					if (xIndexMax < xIndexR) {
						const uint8* s = src + xWeights[xIndexR].index;
						const uint8* sBottom = s + srcBPR;
						d[0] = (s[0] * wTop + sBottom[0] * wBottom) >> 8;
						d[1] = (s[1] * wTop + sBottom[1] * wBottom) >> 8;
						d[2] = (s[2] * wTop + sBottom[2] * wBottom) >> 8;
					}

					dst += dstBPR;
				}

				// last row of pixels if necessary
				// buffer offset into source (bottom row)
				register const uint8* src
					= srcBuffer.row_ptr(yWeights[y2].index);
				// buffer handle for destination to be incremented per pixel
				register uint8* d = dst;

				if (yMax < y2) {
					for (int32 x = xIndexL; x <= xIndexMax; x++) {
						const uint8* s = src + xWeights[x].index;
						const uint16 wLeft = xWeights[x].weight;
						const uint16 wRight = 255 - wLeft;
						d[0] = (s[0] * wLeft + s[4] * wRight) >> 8;
						d[1] = (s[1] * wLeft + s[5] * wRight) >> 8;
						d[2] = (s[2] * wLeft + s[6] * wRight) >> 8;
						d += 4;
					}
				}

				// pixel in bottom right corner if necessary
				if (yMax < y2 && xIndexMax < xIndexR) {
					const uint8* s = src + xWeights[xIndexR].index;
					*(uint32*)d = *(uint32*)s;
				}
				break;
			}
#endif	// __INTEL__
		}
	} while (fBaseRenderer.next_clip_box());

#ifdef FILTER_INFOS_ON_HEAP
	delete[] xWeights;
	delete[] yWeights;
#endif
//printf("draw bitmap %.5fx%.5f: %lld\n", xScale, yScale, system_time() - now);
}


// _DrawBitmapGeneric32
void
Painter::_DrawBitmapGeneric32(agg::rendering_buffer& srcBuffer,
	double xOffset, double yOffset, double xScale, double yScale,
	BRect viewRect, uint32 options) const
{
	TRACE("Painter::_DrawBitmapGeneric32()\n");
	TRACE("   offset: %.1f, %.1f\n", xOffset, yOffset);
	TRACE("   scale: %.3f, %.3f\n", xScale, yScale);
	TRACE("   viewRect: (%.1f, %.1f) - (%.1f, %.1f)\n",
		viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);
	// AGG pipeline

	// pixel format attached to bitmap
	typedef agg::pixfmt_bgra32 pixfmt_image;
	pixfmt_image pixf_img(srcBuffer);

	agg::trans_affine srcMatrix;
	// NOTE: R5 seems to ignore this offset when drawing bitmaps
	//	srcMatrix *= agg::trans_affine_translation(-actualBitmapRect.left,
	//		-actualBitmapRect.top);

	agg::trans_affine imgMatrix;
	imgMatrix *= agg::trans_affine_translation(xOffset - viewRect.left,
		yOffset - viewRect.top);
	imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
	imgMatrix *= agg::trans_affine_translation(viewRect.left, viewRect.top);
	imgMatrix.invert();

	// image interpolator
	typedef agg::span_interpolator_linear<> interpolator_type;
	interpolator_type interpolator(imgMatrix);

	// scanline allocator
	agg::span_allocator<pixfmt_image::color_type> spanAllocator;

	// image accessor attached to pixel format of bitmap
	typedef agg::image_accessor_clip<pixfmt_image> source_type;
	source_type source(pixf_img, agg::rgba8(0, 0, 0, 0));

	// clip to the current clipping region's frame
	viewRect = viewRect & fClippingRegion->Frame();
	// convert to pixel coords (versus pixel indices)
	viewRect.right++;
	viewRect.bottom++;

	// path enclosing the bitmap
	fPath.remove_all();
	fPath.move_to(viewRect.left, viewRect.top);
	fPath.line_to(viewRect.right, viewRect.top);
	fPath.line_to(viewRect.right, viewRect.bottom);
	fPath.line_to(viewRect.left, viewRect.bottom);
	fPath.close_polygon();

	agg::conv_transform<agg::path_storage> transformedPath(fPath, srcMatrix);
	fRasterizer.reset();
	fRasterizer.add_path(transformedPath);

	if ((options & B_FILTER_BITMAP_BILINEAR) != 0) {
		// image filter (bilinear)
		typedef agg::span_image_filter_rgba_bilinear<
			source_type, interpolator_type> span_gen_type;
		span_gen_type spanGenerator(source, interpolator);

		// render the path with the bitmap as scanline fill
		agg::render_scanlines_aa(fRasterizer, fUnpackedScanline, fBaseRenderer,
			spanAllocator, spanGenerator);
	} else {
		// image filter (nearest neighbor)
		typedef agg::span_image_filter_rgba_nn<
			source_type, interpolator_type> span_gen_type;
		span_gen_type spanGenerator(source, interpolator);

		// render the path with the bitmap as scanline fill
		agg::render_scanlines_aa(fRasterizer, fUnpackedScanline, fBaseRenderer,
			spanAllocator, spanGenerator);
	}
}


// _InvertRect32
void
Painter::_InvertRect32(BRect r) const
{
	int32 width = r.IntegerWidth() + 1;
	for (int32 y = (int32)r.top; y <= (int32)r.bottom; y++) {
		uint8* dst = fBuffer.row_ptr(y);
		dst += (int32)r.left * 4;
		for (int32 i = 0; i < width; i++) {
			dst[0] = 255 - dst[0];
			dst[1] = 255 - dst[1];
			dst[2] = 255 - dst[2];
			dst += 4;
		}
	}
}


// _BlendRect32
void
Painter::_BlendRect32(const BRect& r, const rgb_color& c) const
{
	if (!fValidClipping)
		return;

	uint8* dst = fBuffer.row_ptr(0);
	uint32 bpr = fBuffer.stride();

	int32 left = (int32)r.left;
	int32 top = (int32)r.top;
	int32 right = (int32)r.right;
	int32 bottom = (int32)r.bottom;

	// fill rects, iterate over clipping boxes
	fBaseRenderer.first_clip_box();
	do {
		int32 x1 = max_c(fBaseRenderer.xmin(), left);
		int32 x2 = min_c(fBaseRenderer.xmax(), right);
		if (x1 <= x2) {
			int32 y1 = max_c(fBaseRenderer.ymin(), top);
			int32 y2 = min_c(fBaseRenderer.ymax(), bottom);

			uint8* offset = dst + x1 * 4 + y1 * bpr;
			for (; y1 <= y2; y1++) {
				blend_line32(offset, x2 - x1 + 1, c.red, c.green, c.blue,
					c.alpha);
				offset += bpr;
			}
		}
	} while (fBaseRenderer.next_clip_box());
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
	agg::conv_stroke<VertexSource> stroke(path);
	stroke.width(fPenSize);

	stroke.line_cap(agg_line_cap_mode_for(fLineCapMode));
	stroke.line_join(agg_line_join_mode_for(fLineJoinMode));
	stroke.miter_limit(fMiterLimit);

	if (gSubpixelAntialiasing) {
		fSubpixRasterizer.reset();
		fSubpixRasterizer.add_path(stroke);

		agg::render_scanlines(fSubpixRasterizer,
			fSubpixPackedScanline, fSubpixRenderer);
	} else {
		fRasterizer.reset();
		fRasterizer.add_path(stroke);

		agg::render_scanlines(fRasterizer, fPackedScanline, fRenderer);
	}

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
	if (gSubpixelAntialiasing) {
		fSubpixRasterizer.reset();
		fSubpixRasterizer.add_path(path);
		agg::render_scanlines(fSubpixRasterizer,
			fSubpixPackedScanline, fSubpixRenderer);
	} else {
		fRasterizer.reset();
		fRasterizer.add_path(path);
		agg::render_scanlines(fRasterizer, fPackedScanline, fRenderer);
	}

	return _Clipped(_BoundingBox(path));
}


// _FillPath
template<class VertexSource>
BRect
Painter::_FillPath(VertexSource& path, const BGradient& gradient) const
{
	GTRACE("Painter::_FillPath\n");

	switch(gradient.GetType()) {
		case BGradient::TYPE_LINEAR: {
			GTRACE(("Painter::_FillPath> type == TYPE_LINEAR\n"));
			_FillPathGradientLinear(path, *((const BGradientLinear*) &gradient));
			break;
		}
		case BGradient::TYPE_RADIAL: {
			GTRACE(("Painter::_FillPathGradient> type == TYPE_RADIAL\n"));
			_FillPathGradientRadial(path,
				*((const BGradientRadial*) &gradient));
			break;
		}
		case BGradient::TYPE_RADIAL_FOCUS: {
			GTRACE(("Painter::_FillPathGradient> type == TYPE_RADIAL_FOCUS\n"));
			_FillPathGradientRadialFocus(path,
				*((const BGradientRadialFocus*) &gradient));
			break;
		}
		case BGradient::TYPE_DIAMOND: {
			GTRACE(("Painter::_FillPathGradient> type == TYPE_DIAMOND\n"));
			_FillPathGradientDiamond(path,
				*((const BGradientDiamond*) &gradient));
			break;
		}
		case BGradient::TYPE_CONIC: {
			GTRACE(("Painter::_FillPathGradient> type == TYPE_CONIC\n"));
			_FillPathGradientConic(path,
				*((const BGradientConic*) &gradient));
			break;
		}
		case BGradient::TYPE_NONE: {
			GTRACE(("Painter::_FillPathGradient> type == TYPE_NONE\n"));
			break;
		}
	}

	return _Clipped(_BoundingBox(path));
}


// _MakeGradient
void
Painter::_MakeGradient(const BGradient& gradient, int32 colorCount,
	uint32* colors, int32 arrayOffset, int32 arraySize) const
{
	BGradient::ColorStop* from = gradient.ColorStopAt(0);

	if (!from)
		return;

	// current index into "colors" array
//	int32 index = (int32)floorf(colorCount * from->offset + 0.5)
//		+ arrayOffset;
	int32 index = (int32)floorf(colorCount * from->offset / 255 + 0.5)
		+ arrayOffset;
	if (index > arraySize)
		index = arraySize;
	// Make sure we fill the entire array in case the gradient is outside.
	if (index > 0) {
		uint8* c = (uint8*)&colors[0];
		for (int32 i = 0; i < index; i++) {
			c[0] = from->color.blue;
			c[1] = from->color.green;
			c[2] = from->color.red;
			c[3] = from->color.alpha;
			c += 4;
		}
	}

	// interpolate "from" to "to"
	int32 stopCount = gradient.CountColorStops();
	for (int32 i = 1; i < stopCount; i++) {
		// find the step with the next offset
		BGradient::ColorStop* to = gradient.ColorStopAtFast(i);

		// interpolate
//		int32 offset = (int32)floorf((colorCount - 1) * to->offset + 0.5);
		int32 offset = (int32)floorf((colorCount - 1)
			* to->offset / 255 + 0.5);
		if (offset > colorCount - 1)
			offset = colorCount - 1;
		offset += arrayOffset;
		int32 dist = offset - index;
		if (dist >= 0) {
			int32 startIndex = max_c(index, 0);
			int32 stopIndex = min_c(offset, arraySize - 1);
			uint8* c = (uint8*)&colors[startIndex];
			for (int32 i = startIndex; i <= stopIndex; i++) {
				float f = (float)(offset - i) / (float)(dist + 1);
				float t = 1.0 - f;
				c[0] = (uint8)floorf(from->color.blue * f
					+ to->color.blue * t + 0.5);
				c[1] = (uint8)floorf(from->color.green * f
					+ to->color.green * t + 0.5);
				c[2] = (uint8)floorf(from->color.red * f
					+ to->color.red * t + 0.5);
				c[3] = (uint8)floorf(from->color.alpha * f
					+ to->color.alpha * t + 0.5);
				c += 4;
			}
		}
		index = offset + 1;
		// the current "to" will be the "from" in the next interpolation
		from = to;
	}
	//  make sure we fill the entire array
	if (index < arraySize) {
		int32 startIndex = max_c(index, 0);
		uint8* c = (uint8*)&colors[startIndex];
		for (int32 i = startIndex; i < arraySize; i++) {
			c[0] = from->color.blue;
			c[1] = from->color.green;
			c[2] = from->color.red;
			c[3] = from->color.alpha;
			c += 4;
		}
	}
}


// _MakeGradient
template<class Array>
void
Painter::_MakeGradient(Array& array, const BGradient& gradient) const
{
	for (int i = 0; i < gradient.CountColorStops() - 1; i++) {
		BGradient::ColorStop* from = gradient.ColorStopAtFast(i);
		BGradient::ColorStop* to = gradient.ColorStopAtFast(i + 1);
		agg::rgba8 fromColor(from->color.red, from->color.green,
							 from->color.blue, from->color.alpha);
		agg::rgba8 toColor(to->color.red, to->color.green,
						   to->color.blue, to->color.alpha);
		GTRACE("Painter::_MakeGradient> fromColor(%d, %d, %d) offset = %f\n",
			   fromColor.r, fromColor.g, fromColor.b, from->offset);
		GTRACE("Painter::_MakeGradient> toColor(%d, %d, %d) offset = %f\n",
			   toColor.r, toColor.g, toColor.b, to->offset);
		float dist = to->offset - from->offset;
		GTRACE("Painter::_MakeGradient> dist = %f\n", dist);
		// TODO: Review this... offset should better be on [0..1]
		if (dist > 0) {
			for (int j = (int)from->offset; j <= (int)to->offset; j++) {
				float f = (float)(to->offset - j) / (float)(dist + 1);
				array[j] = toColor.gradient(fromColor, f);
				GTRACE("Painter::_MakeGradient> array[%d](%d, %d, %d)\n",
					   array[j].r, array[j].g, array[j].b);
			}
		}
	}
}


// _CalcLinearGradientTransform
void Painter::_CalcLinearGradientTransform(BPoint startPoint, BPoint endPoint,
	agg::trans_affine& matrix, float gradient_d2) const
{
	float dx = endPoint.x - startPoint.x;
	float dy = endPoint.y - startPoint.y;

	matrix.reset();
	matrix *= agg::trans_affine_scaling(sqrt(dx * dx + dy * dy) / gradient_d2);
	matrix *= agg::trans_affine_rotation(atan2(dy, dx));
	matrix *= agg::trans_affine_translation(startPoint.x, startPoint.y);
	matrix.invert();
}


// _FillPathGradientLinear
template<class VertexSource>
void
Painter::_FillPathGradientLinear(VertexSource& path,
	const BGradientLinear& linear) const
{
	GTRACE("Painter::_FillPathGradientLinear\n");

	BPoint start = linear.Start();
	BPoint end = linear.End();

	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::pod_auto_array<agg::rgba8, 256> color_array_type;
	typedef agg::span_allocator<agg::rgba8> span_allocator_type;
	typedef agg::gradient_x	gradient_func_type;
	typedef agg::span_gradient<agg::rgba8, interpolator_type,
				gradient_func_type, color_array_type> span_gradient_type;
	typedef agg::renderer_scanline_aa<renderer_base, span_allocator_type,
				span_gradient_type> renderer_gradient_type;

	gradient_func_type gradientFunc;
	agg::trans_affine gradientMatrix;
	interpolator_type spanInterpolator(gradientMatrix);
	span_allocator_type spanAllocator;
	color_array_type colorArray;

	_MakeGradient(colorArray, linear);

	span_gradient_type spanGradient(spanInterpolator, gradientFunc, colorArray,
		0, 100);

	renderer_gradient_type gradientRenderer(fBaseRenderer, spanAllocator,
		spanGradient);

	_CalcLinearGradientTransform(start, end, gradientMatrix);

	fRasterizer.reset();
	fRasterizer.add_path(path);
	agg::render_scanlines(fRasterizer, fPackedScanline, gradientRenderer);
}


// _FillPathGradientRadial
template<class VertexSource>
void
Painter::_FillPathGradientRadial(VertexSource& path,
	const BGradientRadial& radial) const
{
	GTRACE("Painter::_FillPathGradientRadial\n");

	BPoint center = radial.Center();
// TODO: finish this
//	float radius = radial.Radius();

	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::pod_auto_array<agg::rgba8, 256> color_array_type;
	typedef agg::span_allocator<agg::rgba8> span_allocator_type;
	typedef agg::gradient_radial gradient_func_type;
	typedef agg::span_gradient<agg::rgba8, interpolator_type,
	gradient_func_type, color_array_type> span_gradient_type;
	typedef agg::renderer_scanline_aa<renderer_base, span_allocator_type,
	span_gradient_type> renderer_gradient_type;

	gradient_func_type gradientFunc;
	agg::trans_affine gradientMatrix;
	interpolator_type spanInterpolator(gradientMatrix);
	span_allocator_type spanAllocator;
	color_array_type colorArray;

	_MakeGradient(colorArray, radial);

	span_gradient_type spanGradient(spanInterpolator, gradientFunc, colorArray,
		0, 100);

	renderer_gradient_type gradientRenderer(fBaseRenderer, spanAllocator,
		spanGradient);

	gradientMatrix.reset();
	gradientMatrix *= agg::trans_affine_translation(center.x, center.y);
	gradientMatrix.invert();

//	_CalcLinearGradientTransform(start, end, gradientMtx);

	fRasterizer.reset();
	fRasterizer.add_path(path);
	agg::render_scanlines(fRasterizer, fPackedScanline, gradientRenderer);
}


// _FillPathGradientRadialFocus
template<class VertexSource>
void
Painter::_FillPathGradientRadialFocus(VertexSource& path,
	const BGradientRadialFocus& focus) const
{
	GTRACE("Painter::_FillPathGradientRadialFocus\n");

	BPoint center = focus.Center();
// TODO: finish this.
//	BPoint focal = focus.Focal();
//	float radius = focus.Radius();

	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::pod_auto_array<agg::rgba8, 256> color_array_type;
	typedef agg::span_allocator<agg::rgba8> span_allocator_type;
	typedef agg::gradient_radial_focus gradient_func_type;
	typedef agg::span_gradient<agg::rgba8, interpolator_type,
	gradient_func_type, color_array_type> span_gradient_type;
	typedef agg::renderer_scanline_aa<renderer_base, span_allocator_type,
	span_gradient_type> renderer_gradient_type;

	gradient_func_type gradientFunc;
	agg::trans_affine gradientMatrix;
	interpolator_type spanInterpolator(gradientMatrix);
	span_allocator_type spanAllocator;
	color_array_type colorArray;

	_MakeGradient(colorArray, focus);

	span_gradient_type spanGradient(spanInterpolator, gradientFunc, colorArray,
		0, 100);

	renderer_gradient_type gradientRenderer(fBaseRenderer, spanAllocator,
		spanGradient);

	gradientMatrix.reset();
	gradientMatrix *= agg::trans_affine_translation(center.x, center.y);
	gradientMatrix.invert();

	//	_CalcLinearGradientTransform(start, end, gradientMatrix);

	fRasterizer.reset();
	fRasterizer.add_path(path);
	agg::render_scanlines(fRasterizer, fPackedScanline, gradientRenderer);
}


// _FillPathGradientDiamond
template<class VertexSource>
void
Painter::_FillPathGradientDiamond(VertexSource& path,
	const BGradientDiamond& diamond) const
{
	GTRACE("Painter::_FillPathGradientDiamond\n");

	BPoint center = diamond.Center();
//	float radius = diamond.Radius();

	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::pod_auto_array<agg::rgba8, 256> color_array_type;
	typedef agg::span_allocator<agg::rgba8> span_allocator_type;
	typedef agg::gradient_diamond gradient_func_type;
	typedef agg::span_gradient<agg::rgba8, interpolator_type,
	gradient_func_type, color_array_type> span_gradient_type;
	typedef agg::renderer_scanline_aa<renderer_base, span_allocator_type,
	span_gradient_type> renderer_gradient_type;

	gradient_func_type gradientFunc;
	agg::trans_affine gradientMatrix;
	interpolator_type spanInterpolator(gradientMatrix);
	span_allocator_type spanAllocator;
	color_array_type colorArray;

	_MakeGradient(colorArray, diamond);

	span_gradient_type spanGradient(spanInterpolator, gradientFunc, colorArray,
		0, 100);

	renderer_gradient_type gradientRenderer(fBaseRenderer, spanAllocator,
		spanGradient);

	gradientMatrix.reset();
	gradientMatrix *= agg::trans_affine_translation(center.x, center.y);
	gradientMatrix.invert();

	//	_CalcLinearGradientTransform(start, end, gradientMatrix);

	fRasterizer.reset();
	fRasterizer.add_path(path);
	agg::render_scanlines(fRasterizer, fPackedScanline, gradientRenderer);
}


// _FillPathGradientConic
template<class VertexSource>
void
Painter::_FillPathGradientConic(VertexSource& path,
	const BGradientConic& conic) const
{
	GTRACE("Painter::_FillPathGradientConic\n");

	BPoint center = conic.Center();
//	float radius = conic.Radius();

	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::pod_auto_array<agg::rgba8, 256> color_array_type;
	typedef agg::span_allocator<agg::rgba8> span_allocator_type;
	typedef agg::gradient_conic gradient_func_type;
	typedef agg::span_gradient<agg::rgba8, interpolator_type,
	gradient_func_type, color_array_type> span_gradient_type;
	typedef agg::renderer_scanline_aa<renderer_base, span_allocator_type,
	span_gradient_type> renderer_gradient_type;

	gradient_func_type gradientFunc;
	agg::trans_affine gradientMatrix;
	interpolator_type spanInterpolator(gradientMatrix);
	span_allocator_type spanAllocator;
	color_array_type colorArray;

	_MakeGradient(colorArray, conic);

	span_gradient_type spanGradient(spanInterpolator, gradientFunc, colorArray,
		0, 100);

	renderer_gradient_type gradientRenderer(fBaseRenderer, spanAllocator,
		spanGradient);

	gradientMatrix.reset();
	gradientMatrix *= agg::trans_affine_translation(center.x, center.y);
	gradientMatrix.invert();

	//	_CalcLinearGradientTransform(start, end, gradientMatrix);

	fRasterizer.reset();
	fRasterizer.add_path(path);
	agg::render_scanlines(fRasterizer, fPackedScanline, gradientRenderer);
}
