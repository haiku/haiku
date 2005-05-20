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
class DrawingModeFactory;
class PatternHandler;
class RenderingBuffer;
class ServerBitmap;
class ServerFont;

// TODO: API transition:
// * most all functions should take a DrawData* context parameter instead
//   of the current pattern argument, that way, each function can
//	 decide for itself, which pieces of information in DrawData it
//   needs -> well I'm not so sure about this, there could also
//   be a DrawData member in Painter fGraphicsState or something...
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

			void				ConstrainClipping(const BRegion& region);
			const BRegion*		ClippingRegion() const
									{ return fClippingRegion; }

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

			void				SetPenSize(float size);
			void				SetDrawingMode(drawing_mode mode);
			void				SetBlendingMode(source_alpha alphaSrcMode,
												alpha_function alphaFncMode);
			void				SetPattern(const pattern& p);

			void				SetPenLocation(const BPoint& location);
			void				SetFont(const ServerFont& font);

								// BView API compatibility (for easier testing)
			void				Sync() {}
	inline	void				MovePenTo(const BPoint& location)
									{ SetPenLocation(location); }

								// painting functions

								// lines
			BRect				StrokeLine(		BPoint a,
												BPoint b,
												DrawData* context);

			BRect				StrokeLine(		BPoint b,
												DrawData* context);

			// return true if the line was either vertical or horizontal
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
			BRect				StrokePolygon(	const BPoint* ptArray,
												int32 numPts,
											    bool  closed = true) const;

			BRect				FillPolygon(	const BPoint* ptArray,
												int32 numPts,
											    bool  closed = true) const;

								// bezier curves
			BRect				StrokeBezier(	const BPoint* controlPoints) const;

			BRect				FillBezier(		const BPoint* controlPoints) const;
	
								// shapes
			BRect				StrokeShape(	/*const */BShape* shape) const;

			BRect				FillShape(		/*const */BShape* shape) const;


								// rects
			BRect				StrokeRect(		const BRect& r) const;

			// strokes a one pixel wide solid rect, no blending
			void				StrokeRect(		const BRect& r,
												const rgb_color& c) const;

			BRect				FillRect(		const BRect& r) const;

			// fills a solid rect with color c, no blending
			void				FillRect(		const BRect& r,
												const rgb_color& c) const;

								// round rects
			BRect				StrokeRoundRect(const BRect& r,
												float xRadius,
												float yRadius) const;

			BRect				FillRoundRect(	const BRect& r,
												float xRadius,
												float yRadius) const;

								// ellipses
			BRect				StrokeEllipse(	BPoint center,
												float xRadius,
												float yRadius) const;

			BRect				FillEllipse(	BPoint center,
												float xRadius,
												float yRadius) const;

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
			BRect				FillRegion(		const BRegion* region) const;

			BRect				InvertRect(		const BRect& r) const;

			BRect				BoundingBox(	const char* utf8String,
												uint32 length,
												const BPoint& baseLine) const;

			float				StringWidth(	const char* utf8String,
												uint32 length) const;

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
			void				_UpdateDrawingMode();
			void				_SetRendererColor(const rgb_color& color) const;

								// drawing functions stroke/fill
			BRect				_DrawTriangle(	BPoint pt1,
												BPoint pt2,
												BPoint pt3,
												bool fill) const;
			BRect				_DrawEllipse(	BPoint center,
												float xRadius,
												float yRadius,
												bool fill) const;
			BRect				_DrawShape(		/*const */BShape* shape,
												bool fill) const;
			BRect				_DrawPolygon(	const BPoint* ptArray,
												int32 numPts,
											    bool  closed,
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
			BRect				_StrokePath(VertexSource& path) const;
			template<class VertexSource>
			BRect				_FillPath(VertexSource& path) const;

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

	float						fPenSize;
	BRegion*					fClippingRegion;
	bool						fValidClipping;
	drawing_mode				fDrawingMode;
	source_alpha				fAlphaSrcMode;
	alpha_function				fAlphaFncMode;
	BPoint						fPenLocation;

	DrawingModeFactory*			fDrawingModeFactory;
	PatternHandler*				fPatternHandler;

	ServerFont					fFont;
	// a class handling rendering and caching of glyphs
	// it is setup to load from a specific Freetype supported
	// font file, it uses the FontManager to locate a file
	// by Family and Style
	AGGTextRenderer*			fTextRenderer;
	uint32						fLastFamilyAndStyle;
	float						fLastRotation;
	float						fLastShear;
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


