/*
 * Copyright 2005-2007, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
 * rendering pipe-lines for stroke, fills, bitmap and text rendering.
 *
 */
#ifndef PAINTER_H
#define PAINTER_H


#include "AGGTextRenderer.h"
#include "FontManager.h"
#include "PatternHandler.h"
#include "ServerFont.h"

#include "defines.h"

#include <agg_conv_curve.h>
#include <agg_path_storage.h>

#include <Font.h>
#include <Rect.h>


class BBitmap;
class BRegion;
class BGradient;
class BGradientLinear;
class BGradientRadial;
class BGradientRadialFocus;
class BGradientDiamond;
class BGradientConic;
class DrawState;
class FontCacheReference;
class RenderingBuffer;
class ServerBitmap;
class ServerFont;
class Transformable;


class Painter {
public:
								Painter();
	virtual						~Painter();

								// frame buffer stuff
			void				AttachToBuffer(RenderingBuffer* buffer);
			void				DetachFromBuffer();
			BRect				Bounds() const;

			void				ConstrainClipping(const BRegion* region);
			const BRegion*		ClippingRegion() const
									{ return fClippingRegion; }

			void				SetDrawState(const DrawState* data,
									int32 xOffset = 0,
									int32 yOffset = 0);

								// object settings
			void				SetHighColor(const rgb_color& color);
	inline	rgb_color			HighColor() const
									{ return fPatternHandler.HighColor(); }

			void				SetLowColor(const rgb_color& color);
	inline	rgb_color			LowColor() const
									{ return fPatternHandler.LowColor(); }

			void				SetPenSize(float size);
	inline	float				PenSize() const
									{ return fPenSize; }
			void				SetStrokeMode(cap_mode lineCap,
									join_mode joinMode, float miterLimit);
			void				SetPattern(const pattern& p,
									bool drawingText = false);
	inline	pattern				Pattern() const
									{ return *fPatternHandler.GetR5Pattern(); }
			void				SetDrawingMode(drawing_mode mode);
	inline	drawing_mode		DrawingMode() const
									{ return fDrawingMode; }
			void				SetBlendingMode(source_alpha srcAlpha,
									alpha_function alphaFunc);

			void				SetFont(const ServerFont& font);
			void				SetFont(const DrawState* state);
	inline	const ServerFont&	Font() const
									{ return fTextRenderer.Font(); }

								// painting functions

								// lines
			void				StrokeLine(BPoint a, BPoint b);

			// returns true if the line was either vertical or horizontal
			// draws a solid one pixel wide line of color c, no blending
			bool				StraightLine(BPoint a, BPoint b,
									const rgb_color& c) const;

								// triangles
			BRect				StrokeTriangle(BPoint pt1, BPoint pt2,
									BPoint pt3) const;

			BRect				FillTriangle(BPoint pt1, BPoint pt2,
									BPoint pt3) const;
			BRect				FillTriangle(BPoint pt1, BPoint pt2,
									BPoint pt3,
									const BGradient& gradient) const;

								// polygons
			BRect				DrawPolygon(BPoint* ptArray, int32 numPts,
									bool filled, bool closed) const;
			BRect				FillPolygon(BPoint* ptArray, int32 numPts,
									const BGradient& gradient,
									bool closed) const;

								// bezier curves
			BRect				DrawBezier(BPoint* controlPoints,
									bool filled) const;
			BRect				FillBezier(BPoint* controlPoints,
									const BGradient& gradient) const;

								// shapes
			BRect				DrawShape(const int32& opCount,
									const uint32* opList, const int32& ptCount,
									const BPoint* ptList, bool filled,
									const BPoint& viewToScreenOffset,
									float viewScale) const;
			BRect				FillShape(const int32& opCount,
									const uint32* opList, const int32& ptCount,
									const BPoint* ptList,
									const BGradient& gradient,
									const BPoint& viewToScreenOffset,
									float viewScale) const;

								// rects
			BRect				StrokeRect(const BRect& r) const;

			// strokes a one pixel wide solid rect, no blending
			void				StrokeRect(const BRect& r,
									const rgb_color& c) const;

			BRect				FillRect(const BRect& r) const;
			BRect				FillRect(const BRect& r,
									const BGradient& gradient) const;

			// fills a solid rect with color c, no blending
			void				FillRect(const BRect& r,
									const rgb_color& c) const;

			// fills a rect with a linear gradient, the caller should be
			// sure that the gradient is indeed vertical. The start point of
			// the gradient should be above the end point, or this function
			// will not draw anything.
			void				FillRectVerticalGradient(BRect r,
									const BGradientLinear& gradient) const;

			// fills a solid rect with color c, no blending, no clipping
			void				FillRectNoClipping(const clipping_rect& r,
									const rgb_color& c) const;

								// round rects
			BRect				StrokeRoundRect(const BRect& r, float xRadius,
									float yRadius) const;

			BRect				FillRoundRect(const BRect& r, float xRadius,
									float yRadius) const;
			BRect				FillRoundRect(const BRect& r, float xRadius,
									float yRadius,
									const BGradient& gradient) const;

								// ellipses
			void				AlignEllipseRect(BRect* rect,
									bool filled) const;

			BRect				DrawEllipse(BRect r, bool filled) const;
			BRect				FillEllipse(BRect r,
									const BGradient& gradient) const;

								// arcs
			BRect				StrokeArc(BPoint center, float xRadius,
									float yRadius, float angle,
									float span) const;

			BRect				FillArc(BPoint center, float xRadius,
									float yRadius, float angle,
									float span) const;
			BRect				FillArc(BPoint center, float xRadius,
									float yRadius, float angle, float span,
									const BGradient& gradient) const;

								// strings
			BRect				DrawString(const char* utf8String,
									uint32 length, BPoint baseLine,
									const escapement_delta* delta,
									FontCacheReference* cacheReference = NULL);
			BRect				DrawString(const char* utf8String,
									uint32 length, const BPoint* offsets,
									FontCacheReference* cacheReference = NULL);

			BRect				BoundingBox(const char* utf8String,
									uint32 length, BPoint baseLine,
									BPoint* penLocation,
									const escapement_delta* delta,
									FontCacheReference* cacheReference
										= NULL) const;
			BRect				BoundingBox(const char* utf8String,
									uint32 length, const BPoint* offsets,
									BPoint* penLocation,
									FontCacheReference* cacheReference
										= NULL) const;

			float				StringWidth(const char* utf8String,
									uint32 length,
									const escapement_delta* delta = NULL);


								// bitmaps
			BRect				DrawBitmap(const ServerBitmap* bitmap,
									BRect bitmapRect, BRect viewRect,
									uint32 options) const;

								// some convenience stuff
			BRect				FillRegion(const BRegion* region) const;
			BRect				FillRegion(const BRegion* region,
									const BGradient& gradient) const;

			BRect				InvertRect(const BRect& r) const;

	inline	BRect				ClipRect(BRect rect) const;
	inline	BRect				AlignAndClipRect(BRect rect) const;


private:
			void				_Transform(BPoint* point,
									bool centerOffset = true) const;
			BPoint				_Transform(const BPoint& point,
									bool centerOffset = true) const;
			BRect				_Clipped(const BRect& rect) const;

			void				_UpdateFont() const;
			void				_UpdateLineWidth();
			void				_UpdateDrawingMode(bool drawingText = false);
			void				_SetRendererColor(const rgb_color& color) const;

								// drawing functions stroke/fill
			BRect				_DrawTriangle(BPoint pt1, BPoint pt2,
									BPoint pt3, bool fill) const;

			template<typename sourcePixel>
			void				_TransparentMagicToAlpha(sourcePixel *buffer,
									uint32 width, uint32 height,
									uint32 sourceBytesPerRow,
									sourcePixel transparentMagic,
									BBitmap *output) const;

			void				_DrawBitmap(agg::rendering_buffer& srcBuffer,
									color_space format,
									BRect actualBitmapRect,
									BRect bitmapRect, BRect viewRect,
									uint32 bitmapFlags) const;
			template <class F>
			void				_DrawBitmapNoScale32( F copyRowFunction,
									uint32 bytesPerSourcePixel,
									agg::rendering_buffer& srcBuffer,
									int32 xOffset, int32 yOffset,
									BRect viewRect) const;
			void				_DrawBitmapNearestNeighborCopy32(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const;
			void				_DrawBitmapBilinearCopy32(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const;
			void				_DrawBitmapGeneric32(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect,
									uint32 bitmapFlags) const;

			void				_InvertRect32(BRect r) const;
			void				_BlendRect32(const BRect& r,
									const rgb_color& c) const;


			template<class VertexSource>
			BRect				_BoundingBox(VertexSource& path) const;

			template<class VertexSource>
			BRect				_StrokePath(VertexSource& path) const;
			template<class VertexSource>
			BRect				_FillPath(VertexSource& path) const;
			void				_CalcLinearGradientTransform(BPoint startPoint,
									BPoint endPoint, agg::trans_affine& mtx,
									float gradient_d2 = 100.0f) const;

			void				_MakeGradient(const BGradient& gradient,
									int32 colorCount, uint32* colors,
									int32 arrayOffset, int32 arraySize) const;

			template<class Array>
			void				_MakeGradient(Array& array,
									const BGradient& gradient) const;
			template<class VertexSource>
			BRect				_FillPath(VertexSource& path,
									const BGradient& gradient) const;
			template<class VertexSource>
			void				_FillPathGradientLinear(VertexSource& path,
									const BGradientLinear& linear) const;
			template<class VertexSource>
			void				_FillPathGradientRadial(VertexSource& path,
									const BGradientRadial& radial) const;
			template<class VertexSource>
			void				_FillPathGradientRadialFocus(VertexSource& path,
									const BGradientRadialFocus& focus) const;
			template<class VertexSource>
			void				_FillPathGradientDiamond(VertexSource& path,
									const BGradientDiamond& diamond) const;
			template<class VertexSource>
			void				_FillPathGradientConic(VertexSource& path,
									const BGradientConic& conic) const;

	mutable	agg::rendering_buffer fBuffer;

	// AGG rendering and rasterization classes
			pixfmt				fPixelFormat;
	mutable	renderer_base		fBaseRenderer;

	mutable	scanline_unpacked_type fUnpackedScanline;
	mutable	scanline_packed_type fPackedScanline;
	mutable	scanline_packed_subpix_type fSubpixPackedScanline;
	mutable	scanline_unpacked_subpix_type fSubpixUnpackedScanline;
	mutable	rasterizer_subpix_type fSubpixRasterizer;
	mutable	rasterizer_type		fRasterizer;
	mutable	renderer_subpix_type fSubpixRenderer;
	mutable	renderer_type		fRenderer;
	mutable	renderer_bin_type	fRendererBin;

	mutable	agg::path_storage	fPath;
	mutable	agg::conv_curve<agg::path_storage> fCurve;

	// for internal coordinate rounding/transformation
			bool				fSubpixelPrecise : 1;
			bool				fValidClipping : 1;
			bool				fDrawingText : 1;
			bool				fAttached : 1;

			float				fPenSize;
			const BRegion*		fClippingRegion;
			drawing_mode		fDrawingMode;
			source_alpha		fAlphaSrcMode;
			alpha_function		fAlphaFncMode;
			cap_mode			fLineCapMode;
			join_mode			fLineJoinMode;
			float				fMiterLimit;

			PatternHandler		fPatternHandler;

	// a class handling rendering and caching of glyphs
	// it is setup to load from a specific Freetype supported
	// font file which it gets from ServerFont
	mutable	AGGTextRenderer		fTextRenderer;
};


inline BRect
Painter::ClipRect(BRect rect) const
{
	rect.left = floorf(rect.left);
	rect.top = floorf(rect.top);
	rect.right = ceilf(rect.right);
	rect.bottom = ceilf(rect.bottom);
	return _Clipped(rect);
}


inline BRect
Painter::AlignAndClipRect(BRect rect) const
{
	rect.left = floorf(rect.left);
	rect.top = floorf(rect.top);
	if (fSubpixelPrecise) {
		rect.right = ceilf(rect.right);
		rect.bottom = ceilf(rect.bottom);
	} else {
		rect.right = floorf(rect.right);
		rect.bottom = floorf(rect.bottom);
	}
	return _Clipped(rect);
}


#endif // PAINTER_H
