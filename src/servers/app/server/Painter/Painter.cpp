// Painter.cpp

#include <stdio.h>

#include <Bitmap.h>
#include <Region.h>

#include <agg_bezier_arc.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>
#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_rounded_rect.h>
#include <agg_span_image_filter_rgba32.h>
#include <agg_span_interpolator_linear.h>

#include "AGGTextRenderer.h"
#include "DrawingModeFactory.h"
#include "FontManager.h"
#include "PatternHandler.h"
#include "RenderingBuffer.h"
#include "ShapeConverter.h"

#include "Painter.h"

int
roundf(float v)
{
	if (v >= 0.0)
		return (int)floorf(v + 0.5);
	else
		return (int)floorf(v - 0.5);
}

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
	  fSubpixelPrecise(false),
	  fScale(1.0),
	  fPenSize(1.0),
	  fOrigin(0.0, 0.0),
	  fClippingRegion(NULL),
	  fDrawingMode(B_OP_COPY),
	  fPenLocation(0.0, 0.0),
	  fPatternHandler(new PatternHandler()),
	  fFont(be_plain_font),
	  fTextRenderer(new AGGTextRenderer())
{
	_UpdateFont();
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
	if (buffer && buffer->InitCheck() >= B_OK) {
		// clean up previous stuff
		_MakeEmpty();

		fBuffer = new agg::rendering_buffer();
		fBuffer->attach((uint8*)buffer->Bits(),
						buffer->Width(),
						buffer->Height(),
						buffer->BytesPerRow());

		fPixelFormat = new pixfmt(*fBuffer, fPatternHandler);
		fPixelFormat->set_drawing_mode(DrawingModeFactory::DrawingModeFor(fDrawingMode));

		fBaseRenderer = new renderer_base(*fPixelFormat);

		// These are the AGG renderes and rasterizes which
		// will be used for stroking paths
rgb_color color = fPatternHandler->HighColor();
#if ALIASED_DRAWING
		fOutlineRenderer = new outline_renderer_type(*fBaseRenderer);

		fOutlineRenderer->line_color(agg::rgba(color.red / 255.0,
											   color.green / 255.0,
											   color.blue / 255.0));

		fOutlineRasterizer = new outline_rasterizer_type(*fOutlineRenderer);
#else
		float width = fPenSize;
		_Transform(&width);
		agg::line_profile_aa prof;
        prof.width(width);

		fOutlineRenderer = new outline_renderer_type(*fBaseRenderer, prof);
        fOutlineRenderer->color(agg::rgba(color.red / 255.0,
										  color.green / 255.0,
										  color.blue / 255.0));


		fOutlineRasterizer = new outline_rasterizer_type(*fOutlineRenderer);
#endif

		// the renderer used for filling paths
		fRenderer = new renderer_type(*fBaseRenderer);
		fRasterizer = new rasterizer_type();
		fScanline = new scanline_type();

#if ALIASED_DRAWING
		fRasterizer->gamma(agg::gamma_threshold(0.5));
#endif

        fRenderer->color(agg::rgba(color.red / 255.0,
								   color.green / 255.0,
								   color.blue / 255.0));

		// These are renderers needed for drawing text
		fFontRendererSolid = new font_renderer_solid_type(*fBaseRenderer);

		fFontRendererBin = new font_renderer_bin_type(*fBaseRenderer);


		_RebuildClipping();
	}
}

// DetachFromBuffer
void
Painter::DetachFromBuffer()
{
	_MakeEmpty();
}

// #pragma mark -

// ConstrainClipping
void
Painter::ConstrainClipping(const BRegion& region)
{
	// The idea is that if the clipping region was
	// never constrained, there is *no* clipping.
	// This is of course different from having
	// an *empty* clipping region.
	if (!fClippingRegion)
		fClippingRegion = new BRegion(region);
	else
		*fClippingRegion = region;
	_RebuildClipping();
}

// SetHighColor
void
Painter::SetHighColor(const rgb_color& color)
{
	fPatternHandler->SetHighColor(color);

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

// SetLowColor
void
Painter::SetLowColor(const rgb_color& color)
{
	fPatternHandler->SetLowColor(color);;
}

// SetScale
void
Painter::SetScale(float scale)
{
	fScale = scale;
	_RebuildClipping();
}

// SetScale
void
Painter::SetPenSize(float size)
{
	fPenSize = size;
}

// SetOrigin
void
Painter::SetOrigin(const BPoint& origin)
{
	// NOTE: The BeBook says that the coordinate system
	// of a view cannot be changed during an update, because
	// it would mess up the clipping, and this is indeed
	// what would happen in this implementation as well.
	// I don't know yet what actually happens if you still
	// try to call SetOrigin() from within BView::Draw()
	fOrigin = origin;
	_RebuildClipping();
}

// SetDrawingMode
void
Painter::SetDrawingMode(drawing_mode mode)
{
	if (fDrawingMode != mode) {
		fDrawingMode = mode;
		if (fPixelFormat) {
			fPixelFormat->set_drawing_mode(DrawingModeFactory::DrawingModeFor(fDrawingMode));
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
Painter::SetFont(const BFont& font)
{
	fFont = font;
	_UpdateFont();
}

// #pragma mark -

// StrokeLine
void
Painter::StrokeLine(BPoint a, BPoint b, const pattern& p)
{
	_Transform(&a);
	_Transform(&b);

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, b.y);

	_StrokePath(path, p);

	SetPenLocation(b);
}

// StrokeLine
void
Painter::StrokeLine(BPoint b, const pattern& p)
{
	// TODO: move this function elsewhere
	StrokeLine(fPenLocation, b);
}

// #pragma mark -

// StrokeTriangle
void
Painter::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, const pattern& p) const
{
	_DrawTriangle(pt1, pt2, pt3, p, false);
}

// FillTriangle
void
Painter::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, const pattern& p) const
{
	_DrawTriangle(pt1, pt2, pt3, p, true);
}

// StrokePolygon
void
Painter::StrokePolygon(const BPoint* ptArray, int32 numPts,
					   bool closed, const pattern& p) const
{
	_DrawPolygon(ptArray, numPts, closed, p, false);
}

// FillPolygon
void
Painter::FillPolygon(const BPoint* ptArray, int32 numPts,
					   bool closed, const pattern& p) const
{
	_DrawPolygon(ptArray, numPts, closed, p, true);
}

// StrokeBezier
void
Painter::StrokeBezier(const BPoint* controlPoints, const pattern& p) const
{
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

	_StrokePath(path, p);
}

// FillBezier
void
Painter::FillBezier(const BPoint* controlPoints, const pattern& p) const
{
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

	_FillPath(path, p);
}

// StrokeShape
void
Painter::StrokeShape(/*const */BShape* shape, const pattern& p) const
{
	_DrawShape(shape, p, false);
}

// FillShape
void
Painter::FillShape(/*const */BShape* shape, const pattern& p) const
{
	_DrawShape(shape, p, true);
}

// StrokeRect
void
Painter::StrokeRect(const BRect& r, const pattern& p) const
{
	BPoint a(r.left, r.top);
	BPoint b(r.right, r.top);
	BPoint c(r.right, r.bottom);
	BPoint d(r.left, r.bottom);
	_Transform(&a);
	_Transform(&b);
	_Transform(&c);
	_Transform(&d);

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, b.y);
	path.line_to(c.x, c.y);
	path.line_to(d.x, d.y);
	path.close_polygon();

	_StrokePath(path, p);
}

// FillRect
void
Painter::FillRect(const BRect& r, const pattern& p) const
{
	BPoint a(r.left, r.top);
	BPoint b(r.right, r.top);
	BPoint c(r.right, r.bottom);
	BPoint d(r.left, r.bottom);
	_Transform(&a, false);
	_Transform(&b, false);
	_Transform(&c, false);
	_Transform(&d, false);

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	b.x += 1.0;
	c.x += 1.0;
	c.y += 1.0;
	d.y += 1.0;

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, b.y);
	path.line_to(c.x, c.y);
	path.line_to(d.x, d.y);
	path.close_polygon();

	_FillPath(path, p);
}

// StrokeRoundRect
void
Painter::StrokeRoundRect(const BRect& r, float xRadius, float yRadius,
						 const pattern& p) const
{
	BPoint lt(r.left, r.top);
	BPoint rb(r.right, r.bottom);
	_Transform(&lt);
	_Transform(&rb);

	_Transform(&xRadius);
	_Transform(&yRadius);

	agg::rounded_rect rect;
	rect.rect(lt.x, lt.y, rb.x, rb.y);
	rect.radius(xRadius, yRadius);

//	agg::conv_curve<agg::rounded_rect> path(rect);

//	_StrokePath(path, p);
	_StrokePath(rect, p);
}

// FillRoundRect
void
Painter::FillRoundRect(const BRect& r, float xRadius, float yRadius,
					   const pattern& p) const
{
	BPoint lt(r.left, r.top);
	BPoint rb(r.right, r.bottom);
	_Transform(&lt, false);
	_Transform(&rb, false);

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	rb.x += 1.0;
	rb.y += 1.0;

	_Transform(&xRadius);
	_Transform(&yRadius);

	agg::rounded_rect rect;
	rect.rect(lt.x, lt.y, rb.x, rb.y);
	rect.radius(xRadius, yRadius);

	agg::conv_curve<agg::rounded_rect> path(rect);

	_FillPath(path, p);
}
									
// StrokeEllipse
void
Painter::StrokeEllipse(BPoint center, float xRadius, float yRadius,
					   const pattern& p) const
{
	_DrawEllipse(center, xRadius, yRadius, p, false);
}

// FillEllipse
void
Painter::FillEllipse(BPoint center, float xRadius, float yRadius,
					 const pattern& p) const
{
	_DrawEllipse(center, xRadius, yRadius, p, true);
}

// StrokeArc
void
Painter::StrokeArc(BPoint center, float xRadius, float yRadius,
				   float angle, float span, const pattern& p) const
{
	_Transform(&center);
	_Transform(&xRadius);
	_Transform(&yRadius);

	double angleRad = (angle * PI) / 180.0;
	double spanRad = (span * PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius,
						-angleRad, -spanRad);

	agg::conv_curve<agg::bezier_arc> path(arc);

	_StrokePath(path, p);
}

// FillArc
void
Painter::FillArc(BPoint center, float xRadius, float yRadius,
				 float angle, float span, const pattern& p) const
{
	_Transform(&center);
	_Transform(&xRadius);
	_Transform(&yRadius);

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

	_FillPath(path, p);
}

// #pragma mark -

// DrawString
void
Painter::DrawString(const char* utf8String, BPoint baseLine) const
{
	fPatternHandler->SetPattern(B_SOLID_HIGH);

	_Transform(&baseLine);

	Transformable transform;
// TODO: convert BFont::Shear(), which is in degrees 45°...135° to whatever AGG is using
//	transform.ShearBy(B_ORIGIN, fFont.Shear(), 0.0);
	transform.RotateBy(B_ORIGIN, -fFont.Rotation());
	transform.ScaleBy(B_ORIGIN, fScale, fScale);
	transform.TranslateBy(baseLine);

	fTextRenderer->RenderString(utf8String,
								fFontRendererSolid,
								fFontRendererBin,
								transform);
}

// #pragma mark -

// DrawBitmap
void
Painter::DrawBitmap(const BBitmap* bitmap,
					BRect bitmapRect, BRect viewRect) const
{
typedef agg::span_allocator<agg::rgba8> span_alloc_type;
typedef agg::span_interpolator_linear<> interpolator_type;
typedef agg::span_image_filter_rgba32_nn<agg::order_bgra32,
										 interpolator_type> span_gen_type;
typedef agg::renderer_scanline_aa<renderer_base, span_gen_type> image_renderer_type;

	if (bitmap && bitmap->IsValid()
		&& bitmapRect.IsValid() && bitmapRect.Intersects(bitmap->Bounds())
		&& viewRect.IsValid()) {

		// calculate the scaling
		double xScale = (viewRect.Width() + 1) / (bitmapRect.Width() + 1);
		double yScale = (viewRect.Height() + 1) / (bitmapRect.Height() + 1);

		float xOffset = viewRect.left - (bitmapRect.left * xScale);
		float yOffset = viewRect.top - (bitmapRect.top * yScale);

		BRect actualBitmapRect = bitmap->Bounds();

		// constrain rect to passed bitmap bounds
		bitmapRect = bitmapRect & actualBitmapRect;


		// source rendering buffer
		agg::rendering_buffer srcBuffer;
		srcBuffer.attach((uint8*)bitmap->Bits(),
						 actualBitmapRect.IntegerWidth() + 1,
						 actualBitmapRect.IntegerHeight() + 1,
						 bitmap->BytesPerRow());

		agg::trans_affine srcMatrix;
		srcMatrix *= agg::trans_affine_translation(fOrigin.x, fOrigin.y);
		srcMatrix *= agg::trans_affine_scaling(fScale, fScale);

		agg::trans_affine imgMatrix;
// TODO: What about bitmap bounds that have a top-left corner other than B_ORIGIN?!?
// Ignoring it produces the same result as BView though...
//		imgMatrix *= agg::trans_affine_translation(actualBitmapRect.left, actualBitmapRect.top);
		imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
		imgMatrix *= agg::trans_affine_translation(xOffset, yOffset);
		imgMatrix *= agg::trans_affine_translation(fOrigin.x, fOrigin.y);
		imgMatrix *= agg::trans_affine_scaling(fScale, fScale);
		imgMatrix.invert();
		
		span_alloc_type sa;
		interpolator_type interpolator(imgMatrix);

		span_gen_type sg(sa, srcBuffer, agg::rgba(0, 0, 0, 0), interpolator);

		image_renderer_type ri(*fBaseRenderer, sg);

		agg::rasterizer_scanline_aa<> pf;
		agg::scanline_u8 sl;

		// path encloses image
		agg::path_storage path;
		path.move_to(viewRect.left, viewRect.top);
		path.line_to(viewRect.right + 1, viewRect.top);
		path.line_to(viewRect.right + 1, viewRect.bottom + 1);
		path.line_to(viewRect.left, viewRect.bottom + 1);
		path.line_to(viewRect.left, viewRect.top);

		agg::conv_transform<agg::path_storage> tr(path, srcMatrix);

		pf.add_path(tr);
		agg::render_scanlines(pf, sl, ri);
	}
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
	*point += fOrigin;
	// rounding
	if (!fSubpixelPrecise) {
		// TODO: validate usage of floor() for values < 0
		point->x = floorf(point->x);
		point->y = floorf(point->y);
	}
	// apply the scale
	point->x *= fScale;
	point->y *= fScale;
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

// _Transform
void
Painter::_Transform(float* width) const
{
	*width *= fScale;
	if (*width < 1)
		*width = 1;
}

// _Transform
float
Painter::_Transform(const float& width) const
{
	float w = width * fScale;
	if (w < 1)
		w = 1;
	return w;
}

// _RebuildClipping
void
Painter::_RebuildClipping()
{
	if (fBaseRenderer) {
		fBaseRenderer->reset_clipping(!fClippingRegion);
		if (fClippingRegion) {
			int32 count = fClippingRegion->CountRects();
			for (int32 i = 0; i < count; i++) {
				BRect r = fClippingRegion->RectAt(i);
				// NOTE: The rounding here appears to give somewhat
				// different results compared to Be's implementation,
				// though I was unable to figure out the difference
				BPoint lt(r.LeftTop());
				BPoint rb(r.RightBottom());
rb += BPoint(1.0, 1.0);
//				_Transform(&lt, false);
//				_Transform(&rb, false);
lt += fOrigin;
lt.x *= fScale;
lt.y *= fScale;
rb += fOrigin;
rb.x *= fScale;
rb.y *= fScale;
rb -= BPoint(1.0, 1.0);
//				fBaseRenderer->add_clip_box(floorf(lt.x),
//											floorf(lt.y),
//											ceilf(rb.x),
//											ceilf(rb.y));
				fBaseRenderer->add_clip_box(roundf(lt.x),
											roundf(lt.y),
											roundf(rb.x),
											roundf(rb.y));
			}
		}
	}
}

// _UpdateFont
void
Painter::_UpdateFont()
{
	font_family family;
	font_style style;
	fFont.GetFamilyAndStyle(&family, &style);

	bool success = fTextRenderer->SetFamilyAndStyle(family, style);
	if (!success) {
		fprintf(stderr, "unable to set '%s' + '%s'\n", family, style);
		fprintf(stderr, "font is still: '%s' + '%s'\n",
						fTextRenderer->Family(), fTextRenderer->Style());
	}

	fTextRenderer->SetPointSize(fFont.Size());
}

// #pragma mark -

// _DrawTriangle
inline void
Painter::_DrawTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
					   const pattern& p, bool fill) const
{
	_Transform(&pt1);
	_Transform(&pt2);
	_Transform(&pt3);

	agg::path_storage path;

	path.move_to(pt1.x, pt1.y);
	path.line_to(pt2.x, pt2.y);
	path.line_to(pt3.x, pt3.y);

	path.close_polygon();

	if (fill)
		_FillPath(path, p);
	else
		_StrokePath(path, p);
}

// _DrawEllipse
inline void
Painter::_DrawEllipse(BPoint center, float xRadius, float yRadius,
					  const pattern& p, bool fill) const
{
	// TODO: I think the conversion and the offset of
	// pixel centers might not be correct here, and it
	// might even be necessary to treat Fill and Stroke
	// differently, as with Fill-/StrokeRect().
	_Transform(&center);
	_Transform(&xRadius);
	_Transform(&yRadius);

	float width = fPenSize;
	_Transform(&width);

	int32 divisions = (int32)((xRadius + yRadius) * PI) / 2 * (int32)width;

	agg::ellipse path(center.x, center.y, xRadius, yRadius, divisions);

	if (fill)
		_FillPath(path, p);
	else
		_StrokePath(path, p);
}

// _DrawShape
inline void
Painter::_DrawShape(/*const */BShape* shape, const pattern& p, bool fill) const
{
	// TODO: untested
	agg::path_storage path;
	ShapeConverter converter(&path);

	// account for our view coordinate system
	converter.ScaleBy(B_ORIGIN, fScale, fScale);
	converter.TranslateBy(fOrigin);
	// offset locations to center of pixels
	converter.TranslateBy(BPoint(0.5, 0.5));

	converter.Iterate(shape);

	if (fill)
		_FillPath(path, p);
	else
		_StrokePath(path, p);
}

// _DrawPolygon
inline void
Painter::_DrawPolygon(const BPoint* ptArray, int32 numPts,
					  bool closed, const pattern& p, bool fill) const
{
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
			_FillPath(path, p);
		else
			_StrokePath(path, p);
	}
}



// #pragma mark -

// _StrokePath
template<class VertexSource>
void
Painter::_StrokePath(VertexSource& path, const pattern& p) const
{
	fPatternHandler->SetPattern(p);

	float width = fPenSize;
	_Transform(&width);

#if ALIASED_DRAWING
	if (width > 1.0) {
		agg::conv_stroke<VertexSource> stroke(path);
		stroke.width(width);

		fRasterizer->add_path(stroke);
		agg::render_scanlines(*fRasterizer, *fScanline, *fRenderer);
	} else {
		fOutlineRasterizer->add_path(path);
	}
#else
	agg::line_profile_aa prof;
       prof.width(width);
	fOutlineRenderer->profile(prof);
	fOutlineRasterizer->add_path(path);
#endif
}

// _FillPath
template<class VertexSource>
void
Painter::_FillPath(VertexSource& path, const pattern& p) const
{
	fPatternHandler->SetPattern(p);

	fRasterizer->add_path(path);
	agg::render_scanlines(*fRasterizer, *fScanline, *fRenderer);
}
