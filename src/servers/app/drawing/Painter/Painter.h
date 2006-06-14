/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
 * rendering pipe-lines for stroke, fills, bitmap and text rendering.
 *
 */
#ifndef PAINTER_H
#define PAINTER_H

#include "FontManager.h"
#include "RGBColor.h"
#include "ServerFont.h"

#include "defines.h"

#include <Font.h>
#include <Rect.h>


class AGGTextRenderer;
class BBitmap;
class BRegion;
class DrawState;
class PatternHandler;
class RenderingBuffer;
class ServerBitmap;
class ServerFont;
class Transformable;

// TODO: API transition:
// * most all functions should take a DrawState* context parameter instead
//   of the current pattern argument, that way, each function can
//	 decide for itself, which pieces of information in DrawState it
//   needs -> well I'm not so sure about this, there could also
//   be a DrawState member in Painter fGraphicsState or something...
// * Painter itself should be made thread safe. Because no
//   ServerWindow is supposed to draw outside of its clipping region,
//   there is actually no reason to lock the DisplayDriver. Multiple
//   threads drawing in the frame buffer at the same time is actually
//   only bad if their drawing could overlap, but this is already
//   prevented by the clipping regions (access to those needs to be
//   locked).
//   Making Painter thread safe could introduce some overhead, since
//   some of the current members of Painter would need to be created
//   on the stack... I'll have to see about that... On multiple CPU
//   machines though, there would be quite an improvement. In two
//   years from now, most systems will be at least dual CPU

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
											 bool updateFont = false);

								// object settings
			void				SetHighColor(const rgb_color& color);
	inline	void				SetHighColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	inline	void				SetHighColor(const RGBColor& color)
									{ SetHighColor(color.GetColor32()); }
			void				SetLowColor(const rgb_color& color);
	inline	void				SetLowColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	inline	void				SetLowColor(const RGBColor& color)
									{ SetLowColor(color.GetColor32()); }

			void				SetPenSize(float size);
			void				SetPattern(const pattern& p,
										   bool drawingText = false);

			void				SetPenLocation(const BPoint& location);
			BPoint				PenLocation() const
									{ return fPenLocation; }
			void				SetFont(const ServerFont& font);

								// painting functions

								// lines
			BRect				StrokeLine(		BPoint a,
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
												const DrawState* context);


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
			void				_MakeEmpty();

			void				_Transform(BPoint* point,
										   bool centerOffset = true) const;
			BPoint				_Transform(const BPoint& point,
										   bool centerOffset = true) const;
			BRect				_Clipped(const BRect& rect) const;

			void				_UpdateFont();
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

	agg::rendering_buffer*		fBuffer;

	// AGG rendering and rasterization classes
	pixfmt*						fPixelFormat;
	renderer_base*				fBaseRenderer;

	outline_renderer_type*		fOutlineRenderer;
	outline_rasterizer_type*	fOutlineRasterizer;

	scanline_unpacked_type*		fUnpackedScanline;
	scanline_packed_type*		fPackedScanline;
	rasterizer_type*			fRasterizer;
	renderer_type*				fRenderer;
	renderer_bin_type*			fRendererBin;

	agg::line_profile_aa		fLineProfile;

	// for internal coordinate rounding/transformation
	bool						fSubpixelPrecise;

	float						fPenSize;
	BRegion*					fClippingRegion;
	bool						fValidClipping;
	drawing_mode				fDrawingMode;
	bool						fDrawingText;
	source_alpha				fAlphaSrcMode;
	alpha_function				fAlphaFncMode;
	BPoint						fPenLocation;
	cap_mode					fLineCapMode;
	join_mode					fLineJoinMode;
	float						fMiterLimit;

	PatternHandler*				fPatternHandler;

	ServerFont					fFont;
	// a class handling rendering and caching of glyphs
	// it is setup to load from a specific Freetype supported
	// font file which it gets from ServerFont
	AGGTextRenderer*			fTextRenderer;
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


