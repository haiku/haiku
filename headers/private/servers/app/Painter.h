// Painter.h

#ifndef PAINTER_H
#define PAINTER_H

#include <Font.h>
#include <Rect.h>
#include <FontServer.h>
#include <ServerFont.h>

#include "defines.h"
#include "forwarding_pixfmt.h"

#include "RGBColor.h"

class AGGTextRenderer;
class BBitmap;
class BRegion;
class DrawData;
class PatternHandler;
class RenderingBuffer;
class ServerBitmap;
class ServerFont;

class Painter {
 public:
								Painter();
	virtual						~Painter();

								// frame buffer stuff
			void				AttachToBuffer(RenderingBuffer* buffer);
			void				DetachFromBuffer();

			void				ConstrainClipping(const BRegion& region);
			void				SetDrawData(const DrawData* data);

								// object settings
			void				SetHighColor(const rgb_color& color);
	inline	void				SetHighColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	inline	void				SetHighColor(const RGBColor& color)
									{ SetHighColor(color.GetColor32()); }
			void				SetLowColor(const rgb_color& color);
	inline	void				SetLowColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
	inline	void				SetLowColor(const RGBColor& color)
									{ SetLowColor(color.GetColor32()); }

			void				SetScale(float scale);
			void				SetPenSize(float size);
			void				SetOrigin(const BPoint& origin);
			void				SetDrawingMode(drawing_mode mode);
			void				SetBlendingMode(source_alpha alphaSrcMode,
												alpha_function alphaFncMode);
			void				SetPenLocation(const BPoint& location);
			void				SetFont(const BFont& font);
			void				SetFont(const ServerFont& font);

								// BView API compatibility (for easier testing)
			void				Sync() {}
	inline	void				MovePenTo(const BPoint& location)
									{ SetPenLocation(location); }
	inline	void				SetFont(const BFont* font)
									{ if (font) SetFont(*font); }

								// painting functions

								// lines
			BRect				StrokeLine(		BPoint a,
												BPoint b,
												const pattern& p = B_SOLID_HIGH);

			BRect				StrokeLine(		BPoint b,
												const pattern& p = B_SOLID_HIGH);

			// return true if the line was either vertical or horizontal
			// draws a solid one pixel wide line of color c, no blending
			bool				StraightLine(	BPoint a,
												BPoint b,
												const rgb_color& c) const;

								// triangles
			void				StrokeTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3,
												const pattern& p = B_SOLID_HIGH) const;

								// polygons
			void				StrokePolygon(	const BPoint* ptArray,
												int32 numPts,
											    bool  closed = true,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillPolygon(	const BPoint* ptArray,
												int32 numPts,
											    bool  closed = true,
												const pattern& p = B_SOLID_HIGH) const;

								// bezier curves
			void				StrokeBezier(	const BPoint* controlPoints,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillBezier(		const BPoint* controlPoints,
												const pattern& p = B_SOLID_HIGH) const;
	
								// shapes
			void				StrokeShape(	/*const */BShape* shape,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillShape(		/*const */BShape* shape,
												const pattern& p = B_SOLID_HIGH) const;


								// rects
			BRect				StrokeRect(		const BRect& r,
												const pattern& p = B_SOLID_HIGH) const;

			// strokes a one pixel wide solid rect, no blending
			void				StrokeRect(		const BRect& r,
												const rgb_color& c) const;

			BRect				FillRect(		const BRect& r,
												const pattern& p = B_SOLID_HIGH) const;

			// fills a solid rect with color c, no blending
			void				FillRect(		const BRect& r,
												const rgb_color& c) const;

								// round rects
			void				StrokeRoundRect(const BRect& r,
												float xRadius,
												float yRadius,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillRoundRect(	const BRect& r,
												float xRadius,
												float yRadius,
												const pattern& p = B_SOLID_HIGH) const;

								// ellipses
			void				StrokeEllipse(	BPoint center,
												float xRadius,
												float yRadius,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillEllipse(	BPoint center,
												float xRadius,
												float yRadius,
												const pattern& p = B_SOLID_HIGH) const;

								// arcs
			void				StrokeArc(		BPoint center,
												float xRadius,
												float yRadius,
												float angle,
												float span,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillArc(		BPoint center,
												float xRadius,
												float yRadius,
												float angle,
												float span,
												const pattern& p = B_SOLID_HIGH) const;

								// strings
			BRect				DrawChar(		char aChar);

			BRect				DrawChar(		char aChar,
												BPoint baseLine);

			BRect				DrawString(		const char* utf8String,
												uint32 length,
												const escapement_delta* delta = NULL);

			BRect				DrawString(		const char* utf8String,
												uint32 length,
												BPoint baseLine,
												const escapement_delta* delta = NULL);

			BRect				DrawString(		const char* utf8String,
												const escapement_delta* delta = NULL);

			BRect				DrawString(		const char* utf8String,
												BPoint baseLine,
												const escapement_delta* delta = NULL);

								// bitmaps
			void				DrawBitmap(		const BBitmap* bitmap,
												BRect bitmapRect,
												BRect viewRect) const;

			void				DrawBitmap(		const ServerBitmap* bitmap,
												BRect bitmapRect,
												BRect viewRect) const;

								// some convenience stuff
			void				FillRegion(		const BRegion* region,
												const pattern& p = B_SOLID_HIGH) const;

			void				InvertRect(		const BRect& r) const;

			BRect				BoundingBox(	const char* utf8String,
												uint32 length,
												const BPoint& baseLine) const;

	inline	BRect				ClipRect(const BRect& rect) const
									{ return _Clipped(rect); }

 private:
			void				_MakeEmpty();

			void				_Transform(BPoint* point,
										   bool centerOffset = true) const;
			BPoint				_Transform(const BPoint& point,
										   bool centerOffset = true) const;
			void				_Transform(float* width) const;
			float				_Transform(const float& width) const;
			void				_Transform(BRect* rect) const;
			BRect				_Transform(const BRect& rect) const;
			BRect				_Clipped(const BRect& rect) const;

			void				_RebuildClipping();

			void				_UpdateFont();
			void				_UpdateLineWidth();

								// drawing functions stroke/fill
			void				_DrawTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3,
												const pattern& p,
												bool fill) const;
			void				_DrawEllipse(	BPoint center,
												float xRadius,
												float yRadius,
												const pattern& p,
												bool fill) const;
			void				_DrawShape(		/*const */BShape* shape,
												const pattern& p,
												bool fill) const;
			void				_DrawPolygon(	const BPoint* ptArray,
												int32 numPts,
											    bool  closed,
												const pattern& p,
												bool fill) const;

			void				_DrawBitmap(	const agg::rendering_buffer& srcBuffer,
												color_space format,
												BRect actualBitmapRect,
												BRect bitmapRect,
												BRect viewRect) const;
			void				_DrawBitmap32(	const agg::rendering_buffer& srcBuffer,
												BRect actualBitmapRect,
												BRect bitmapRect,
												BRect viewRect) const;

			void				_InvertRect32(BRect r) const;


			template<class VertexSource>
			BRect				_BoundingBox(VertexSource& path) const;

			template<class VertexSource>
			BRect				_StrokePath(VertexSource& path,
											const pattern& p) const;
			template<class VertexSource>
			BRect				_FillPath(VertexSource& path,

										  const pattern& p) const;

			void				_SetPattern(const pattern& p) const;
			void				_SetRendererColor(const rgb_color& color) const;

	agg::rendering_buffer*		fBuffer;

	// AGG rendering and rasterization classes
	pixfmt*						fPixelFormat;
	renderer_base*				fBaseRenderer;

	outline_renderer_type*		fOutlineRenderer;
	outline_rasterizer_type*	fOutlineRasterizer;

	scanline_type*				fScanline;
	rasterizer_type*			fRasterizer;
	renderer_type*				fRenderer;

	font_renderer_solid_type*	fFontRendererSolid;
	font_renderer_bin_type*		fFontRendererBin;

	agg::line_profile_aa		fLineProfile;

	// for internal coordinate rounding/transformation,
	// does not concern rendering
	bool						fSubpixelPrecise;

	float						fScale;
	float						fPenSize;
	BPoint						fOrigin;
	BRegion*					fClippingRegion; // NULL indicates no clipping at all
	drawing_mode				fDrawingMode;
	source_alpha				fAlphaSrcMode;
	alpha_function				fAlphaFncMode;
	BPoint						fPenLocation;
	PatternHandler*				fPatternHandler;

	ServerFont					fFont;
	// a class handling rendering and caching of glyphs
	// it is setup to load from a specific Freetype supported
	// font file, it uses the FontManager to locate a file
	// by Family and Style
	AGGTextRenderer*			fTextRenderer;
	uint32						fLastFamilyAndStyle;
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


