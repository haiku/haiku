// Painter.h

#ifndef PAINTER_H
#define PAINTER_H

#include <Font.h>
#include <Rect.h>

#include "defines.h"
#include "forwarding_pixfmt.h"

class AGGTextRenderer;
class BBitmap;
class BRegion;
class PatternHandler;
class RenderingBuffer;

class Painter {
 public:
								Painter();
	virtual						~Painter();

								// frame buffer stuff
			void				AttachToBuffer(RenderingBuffer* buffer);
			void				DetachFromBuffer();

			void				ConstrainClipping(const BRegion& region);

								// object settings
			void				SetHighColor(const rgb_color& color);
			void				SetHighColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
			void				SetLowColor(const rgb_color& color);
			void				SetLowColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);

			void				SetScale(float scale);
			void				SetPenSize(float size);
			void				SetOrigin(const BPoint& origin);
			void				SetDrawingMode(drawing_mode mode);
			void				SetPenLocation(const BPoint& location);
			void				SetFont(const BFont& font);

								// painting functions

								// lines
			void				StrokeLine(		BPoint a,
												BPoint b,
												const pattern& p = B_SOLID_HIGH);

			void				StrokeLine(		BPoint b,
												const pattern& p = B_SOLID_HIGH);
			
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
			void				StrokeRect(		const BRect& r,
												const pattern& p = B_SOLID_HIGH) const;

			void				FillRect(		const BRect& r,
												const pattern& p = B_SOLID_HIGH) const;

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
			void				DrawString(		const char* utf8String,
												const escapement_delta* delta = NULL);

			void				DrawString(		const char* utf8String,
												BPoint baseLine,
												const escapement_delta* delta = NULL);

								// bitmaps
			void				DrawBitmap(		const BBitmap* bitmap,
												BRect bitmapRect,
												BRect viewRect) const;


			// MISSING:
/*			void				FillRegion(		const BRegion* region,
												const pattern& p = B_SOLID_HIGH);

			void				InvertRect(		BRect r);


								// "screen blits"
			void				CopyBits(		BRect src, BRect dst);


								// more string support
			void				DrawChar(		char aChar);

			void				DrawChar(		char aChar,
												BPoint location);

			void				DrawString(		const char* string,
												int32 length,
												const escapement_delta* delta = NULL);

			void				DrawString(		const char* string,
												int32 length,
												BPoint location,
												const escapement_delta* delta = 0L);*/


 private:
			void				_MakeEmpty();

			void				_Transform(BPoint* point,
										   bool centerOffset = true) const;
			BPoint				_Transform(const BPoint& point,
										   bool centerOffset = true) const;
			void				_Transform(float* width) const;
			float				_Transform(const float& width) const;

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


			template<class VertexSource>
			void				_StrokePath(VertexSource& path,
											const pattern& p) const;
			template<class VertexSource>
			void				_FillPath(VertexSource& path,
										  const pattern& p) const;

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
	BPoint						fPenLocation;
	PatternHandler*				fPatternHandler;

	BFont						fFont;
	// a class handling rendering and caching of glyphs
	// it is setup to load from a specific Freetype supported
	// font file, it uses the FontManager to locate a file
	// by Family and Style
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


