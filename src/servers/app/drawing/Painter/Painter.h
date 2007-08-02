/*
 * Copyright 2005-2007, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
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
#include "RGBColor.h"
#include "ServerFont.h"

#include "defines.h"

#include <agg_conv_curve.h>
#include <agg_path_storage.h>

#include <Font.h>
#include <Rect.h>


class BBitmap;
class BRegion;
class DrawState;
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

			void				ConstrainClipping(const BRegion* region);
			const BRegion*		ClippingRegion() const
									{ return fClippingRegion; }

			void				SetDrawState(const DrawState* data,
											 bool updateFont = false,
											 int32 xOffset = 0,
											 int32 yOffset = 0);

								// object settings
			void				SetHighColor(const rgb_color& color);
	inline	void				SetHighColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	inline	void				SetHighColor(const RGBColor& color)
									{ SetHighColor(color.GetColor32()); }
	inline	rgb_color			HighColor() const
									{ return fPatternHandler.
										HighColor().GetColor32(); }

			void				SetLowColor(const rgb_color& color);
	inline	void				SetLowColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	inline	void				SetLowColor(const RGBColor& color)
									{ SetLowColor(color.GetColor32()); }

	inline	rgb_color			LowColor() const
									{ return fPatternHandler.
										LowColor().GetColor32(); }

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

			void				SetPenLocation(const BPoint& location);
	inline	BPoint				PenLocation() const
									{ return fPenLocation; }
			void				SetFont(const ServerFont& font);
	inline	const ServerFont&	Font() const
									{ return fFont; }

								// painting functions

								// lines
			void				StrokeLine(		BPoint a,
												BPoint b);

			// returns true if the line was either vertical or horizontal
			// draws a solid one pixel wide line of color c, no blending
			bool				StraightLine(	BPoint a,
												BPoint b,
												const rgb_color& c) const;

								// triangles
			BRect				StrokeTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3) const;

			BRect				FillTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3) const;

								// polygons
			BRect				DrawPolygon(	BPoint* ptArray,
												int32 numPts,
											    bool filled,
											    bool closed) const;

								// bezier curves
			BRect				DrawBezier(		BPoint* controlPoints,
												bool filled) const;
	
								// shapes
			BRect				DrawShape(		const int32& opCount,
												const uint32* opList,
												const int32& ptCount,
												const BPoint* ptList,
												bool filled) const;

								// rects
			BRect				StrokeRect(		const BRect& r) const;

			// strokes a one pixel wide solid rect, no blending
			void				StrokeRect(		const BRect& r,
												const rgb_color& c) const;

			BRect				FillRect(		const BRect& r) const;

			// fills a solid rect with color c, no blending
			void				FillRect(		const BRect& r,
												const rgb_color& c) const;
			// fills a solid rect with color c, no blending, no clipping
			void				FillRectNoClipping(const BRect& r,
												const rgb_color& c) const;

								// round rects
			BRect				StrokeRoundRect(const BRect& r,
												float xRadius,
												float yRadius) const;

			BRect				FillRoundRect(	const BRect& r,
												float xRadius,
												float yRadius) const;

								// ellipses
			void				AlignEllipseRect(BRect* rect,
												 bool filled) const;

			BRect				DrawEllipse(	BRect r,
												bool filled) const;

								// arcs
			BRect				StrokeArc(		BPoint center,
												float xRadius,
												float yRadius,
												float angle,
												float span) const;

			BRect				FillArc(		BPoint center,
												float xRadius,
												float yRadius,
												float angle,
												float span) const;

								// strings
			BRect				DrawString(		const char* utf8String,
												uint32 length,
												BPoint baseLine,
												const escapement_delta* delta = NULL);

			BRect				BoundingBox(	const char* utf8String,
												uint32 length,
												BPoint baseLine,
												BPoint* penLocation,
												const escapement_delta* delta = NULL) const;

			float				StringWidth(	const char* utf8String,
												uint32 length,
												const escapement_delta* delta = NULL);


								// bitmaps
			BRect				DrawBitmap(		const ServerBitmap* bitmap,
												BRect bitmapRect,
												BRect viewRect) const;

								// some convenience stuff
			BRect				FillRegion(		const BRegion* region) const;

			BRect				InvertRect(		const BRect& r) const;

	inline	BRect				ClipRect(const BRect& rect) const
									{ return _Clipped(rect); }

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
			BRect				_DrawTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3,
												bool fill) const;

			void				_DrawBitmap(	agg::rendering_buffer& srcBuffer,
												color_space format,
												BRect actualBitmapRect,
												BRect bitmapRect,
												BRect viewRect) const;
			template <class F>
			void				_DrawBitmapNoScale32(
												F copyRowFunction,
												uint32 bytesPerSourcePixel,
												agg::rendering_buffer& srcBuffer,
												int32 xOffset, int32 yOffset,
												BRect viewRect) const;
			void				_DrawBitmapGeneric32(
												agg::rendering_buffer& srcBuffer,
												double xOffset, double yOffset,
												double xScale, double yScale,
												BRect viewRect) const;

			void				_InvertRect32(	BRect r) const;
			void				_BlendRect32(	const BRect& r,
												const rgb_color& c) const;


			template<class VertexSource>
			BRect				_BoundingBox(VertexSource& path) const;

			template<class VertexSource>
			BRect				_StrokePath(VertexSource& path) const;
			template<class VertexSource>
			BRect				_FillPath(VertexSource& path) const;

mutable agg::rendering_buffer	fBuffer;

	// AGG rendering and rasterization classes
	pixfmt						fPixelFormat;
mutable renderer_base			fBaseRenderer;

mutable scanline_unpacked_type	fUnpackedScanline;
mutable scanline_packed_type	fPackedScanline;
mutable rasterizer_type			fRasterizer;
mutable renderer_type			fRenderer;
mutable renderer_bin_type		fRendererBin;

mutable agg::path_storage		fPath;
mutable agg::conv_curve<agg::path_storage> fCurve;

	// for internal coordinate rounding/transformation
	bool						fSubpixelPrecise;

	float						fPenSize;
	const BRegion*				fClippingRegion;
	bool						fValidClipping;
	drawing_mode				fDrawingMode;
	bool						fDrawingText;
	source_alpha				fAlphaSrcMode;
	alpha_function				fAlphaFncMode;
	BPoint						fPenLocation;
	cap_mode					fLineCapMode;
	join_mode					fLineJoinMode;
	float						fMiterLimit;

	PatternHandler				fPatternHandler;

	ServerFont					fFont;
	// a class handling rendering and caching of glyphs
	// it is setup to load from a specific Freetype supported
	// font file which it gets from ServerFont
mutable AGGTextRenderer			fTextRenderer;
};

// SetHighColor
inline void
Painter::SetHighColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	rgb_color color;
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
	SetHighColor(color);
}

// SetLowColor
inline void
Painter::SetLowColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	rgb_color color;
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
	SetLowColor(color);
}


#endif // PAINTER_H


