/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AGGTextRenderer.h"

#include "FontManager.h"
#include "ServerFont.h"
#include "utf8_functions.h"

#include <agg_basics.h>
#include <agg_bounding_rect.h>
#include <agg_conv_segmentator.h>
#include <agg_conv_transform.h>
#include <agg_trans_affine.h>

#include <Bitmap.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <Message.h>
#include <UTF8.h>

#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define FLIP_Y false

#define SHOW_GLYPH_BOUNDS 0

#if SHOW_GLYPH_BOUNDS
#	include <agg_conv_stroke.h>
#	include <agg_path_storage.h>
#endif

// rect_to_int
inline void
rect_to_int(BRect r,
			int32& left, int32& top, int32& right, int32& bottom)
{
	left = (int32)floorf(r.left);
	top = (int32)floorf(r.top);
	right = (int32)ceilf(r.right);
	bottom = (int32)ceilf(r.bottom);
}


inline bool
is_white_space(uint32 charCode)
{
	switch (charCode) {
		case 0x0009:	/* tab */
		case 0x000b:	/* vertical tab */
		case 0x000c:	/* form feed */
		case 0x0020:	/* space */
		case 0x00a0:	/* non breaking space */
		case 0x000a:	/* line feed */
		case 0x000d:	/* carriage return */
		case 0x2028:	/* line separator */
		case 0x2029:	/* paragraph separator */
			return true;
	}

	return false;
}

#define DEFAULT_UNI_CODE_BUFFER_SIZE	2048

// constructor
AGGTextRenderer::AGGTextRenderer()
	: fFontEngine(gFreeTypeLibrary),
	  fFontCache(fFontEngine),

	  fCurves(fFontCache.path_adaptor()),
	  fContour(fCurves),

	  fUnicodeBuffer((char*)malloc(DEFAULT_UNI_CODE_BUFFER_SIZE)),
	  fUnicodeBufferSize(DEFAULT_UNI_CODE_BUFFER_SIZE),

	  fHinted(true),
	  fAntialias(true),
	  fKerning(true),
	  fEmbeddedTransformation()
{
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(FLIP_Y);
}

// destructor
AGGTextRenderer::~AGGTextRenderer()
{
	Unset();
	free(fUnicodeBuffer);
}

// SetFont
bool
AGGTextRenderer::SetFont(const ServerFont &font)
{
	// construct an embedded transformation (rotate & shear)
	fEmbeddedTransformation.Reset();
	fEmbeddedTransformation.ShearBy(B_ORIGIN,
		(90.0 - font.Shear()) * PI / 180.0, 0.0);
	fEmbeddedTransformation.RotateBy(B_ORIGIN,
		-font.Rotation() * PI / 180.0);

	agg::glyph_rendering glyphType =
		fHinted && fEmbeddedTransformation.IsIdentity() ?
			agg::glyph_ren_native_gray8 :
			agg::glyph_ren_outline;

	fFontEngine.load_font(font, glyphType, font.Size());

	return true;
}

// SetHinting
void
AGGTextRenderer::SetHinting(bool hinting)
{
	fHinted = hinting;
	fFontEngine.hinting(fEmbeddedTransformation.IsIdentity() && fHinted);
}

// SetAntialiasing
void
AGGTextRenderer::SetAntialiasing(bool antialiasing)
{
	if (fAntialias != antialiasing) {
		fAntialias = antialiasing;
		if (!fAntialias)
			fRasterizer.gamma(agg::gamma_threshold(0.5));
		else
			fRasterizer.gamma(agg::gamma_power(1.0));
	}
}

// Unset
void
AGGTextRenderer::Unset()
{
	// TODO ? release some kind of reference count on the ServerFont?
}


BRect
AGGTextRenderer::RenderString(const char* string,
							  uint32 length,
							  renderer_type* solidRenderer,
							  renderer_bin_type* binRenderer,
							  const BPoint& baseLine,
							  const BRect& clippingFrame,
							  bool dryRun,
							  BPoint* nextCharPos,
							  const escapement_delta* delta)
{
//printf("RenderString(\"%s\", length: %ld, dry: %d)\n", string, length, dryRun);

	// "bounds" will track the bounding box arround all glyphs that are actually drawn
	// it will be calculated in untransformed coordinates within the loop and then
	// it is transformed to the real location at the exit of the function.
	BRect bounds(0.0, 0.0, -1.0, -1.0);

	Transformable transform(fEmbeddedTransformation);
	transform.TranslateBy(baseLine);

	fCurves.approximation_scale(transform.scale());

	// use a transformation behind the curves
	// (only if glyph->data_type == agg::glyph_data_outline)
	// in the pipeline for the rasterizer
	typedef agg::conv_transform<conv_font_curve_type, agg::trans_affine>
		conv_font_trans_type;
	conv_font_trans_type transformedOutline(fCurves, transform);

	double x  = 0.0;
	double y0 = 0.0;
	double y  = y0;

	double advanceX = 0.0;
	double advanceY = 0.0;
	bool firstLoop = true;

	// for when we bypass the transformation pipeline
	BPoint transformOffset(0.0, 0.0);
	transform.Transform(&transformOffset);

	fFontCache.reset();

	uint32 charCode;
	while ((charCode = UTF8ToCharCode(&string)) > 0) {
		// line break (not supported by R5)
		/*if (charCode == '\n') {
			y0 += LineOffset();
			x = 0.0;
			y = y0;
			advanceX = 0.0;
			advanceY = 0.0;
			continue;
		}*/

		const agg::glyph_cache* glyph = fFontCache.glyph(charCode);
		if (glyph == NULL) {
			fprintf(stderr, "failed to load glyph for 0x%04lx (%c)\n", charCode,
				isprint(charCode) ? (char)charCode : '-');

			continue;
		}

		if (!firstLoop && fKerning)
			fFontCache.add_kerning(&advanceX, &advanceY);

		x += advanceX;
		y += advanceY;

		if (delta)
			x += is_white_space(charCode) ? delta->space : delta->nonspace;

		// "glyphBounds" is the bounds of the glyph transformed
		// by the x y location of the glyph along the base line,
		// it is therefor yet "untransformed".
		const agg::rect& r = glyph->bounds;
		BRect glyphBounds(r.x1 + x, r.y1 + y - 1, r.x2 + x + 1, r.y2 + y + 1);
			// NOTE: "-1"/"+1" converts the glyph bounding box from pixel
			// indices to pixel area coordinates

		// track bounding box
		if (glyphBounds.IsValid())
			bounds = bounds.IsValid() ? bounds | glyphBounds : glyphBounds;

		// render the glyph if this is not a dry run
		if (!dryRun) {
			// init the fontmanager's embedded adaptors
			// NOTE: The initialization for the "location" of
			// the glyph is different depending on wether we
			// deal with non-(rotated/sheared) text, in which
			// case we have a native FT bitmap. For rotated or
			// sheared text, we use AGG vector outlines and 
			// a transformation pipeline, which will be applied
			// _after_ we retrieve the outline, and that's why
			// we simply pass x and y, which are untransformed.

			// "glyphBounds" is now transformed into screen coords
			// in order to stop drawing when we are already outside
			// of the clipping frame
			if (glyph->data_type != agg::glyph_data_outline) {
				// we cannot use the transformation pipeline
				double transformedX = x + transformOffset.x;
				double transformedY = y + transformOffset.y;
				fFontCache.init_embedded_adaptors(glyph,
					transformedX, transformedY);
				glyphBounds.OffsetBy(transformOffset);
			} else {
				fFontCache.init_embedded_adaptors(glyph, x, y);
				glyphBounds = transform.TransformBounds(glyphBounds);
			}

			if (clippingFrame.Intersects(glyphBounds)) {
				switch (glyph->data_type) {
					case agg::glyph_data_mono:
						agg::render_scanlines(fFontCache.mono_adaptor(), 
							fFontCache.mono_scanline(), *binRenderer);
						break;

					case agg::glyph_data_gray8:
						agg::render_scanlines(fFontCache.gray8_adaptor(), 
							fFontCache.gray8_scanline(), *solidRenderer);
						break;

					case agg::glyph_data_outline: {
						fRasterizer.reset();
						// NOTE: this function can be easily extended to handle
						// conversion to contours, so that's why there is a lot of
						// commented out code, I leave it here because I think it
						// will be needed again.

						//if(fabs(0.0) <= 0.01) {
							// For the sake of efficiency skip the
							// contour converter if the weight is about zero.

							fRasterizer.add_path(transformedOutline);
#if SHOW_GLYPH_BOUNDS
	agg::path_storage p;
	p.move_to(glyphBounds.left + 0.5, glyphBounds.top + 0.5);
	p.line_to(glyphBounds.right + 0.5, glyphBounds.top + 0.5);
	p.line_to(glyphBounds.right + 0.5, glyphBounds.bottom + 0.5);
	p.line_to(glyphBounds.left + 0.5, glyphBounds.bottom + 0.5);
	p.close_polygon();
	agg::conv_stroke<agg::path_storage> ps(p);
	ps.width(1.0);
	fRasterizer.add_path(ps);
#endif
						/*} else {
							//fRasterizer.add_path(fContour);
							fRasterizer.add_path(transformedOutline);
						}*/

						agg::render_scanlines(fRasterizer, fScanline, *solidRenderer);
						break;
					}
					default:
						break;
				}
			}
		}

		// increment pen position
		advanceX = glyph->advance_x;
		advanceY = glyph->advance_y;

		firstLoop = false;
	};

	// put pen location behind rendered text
	// (at the baseline of the virtual next glyph)
	if (nextCharPos) {
		nextCharPos->x = x + advanceX;
		nextCharPos->y = y + advanceY;

		transform.Transform(nextCharPos);
	}

	return transform.TransformBounds(bounds);
}


double
AGGTextRenderer::StringWidth(const char* string, uint32 length)
{
	// NOTE: The implementation does not take font rotation (or shear)
	// into account. Just like on R5. Should it ever be desirable to
	// "fix" this, simply use (before "return width;"):
	//
	// BPoint end(width, 0.0);
	// fEmbeddedTransformation.Transform(&end);
	// width = fabs(end.x);
	//
	// Note that shear will not have any influence on the baseline though.

	double width = 0.0;
	uint32 charCode;
	double y  = 0.0;
	const agg::glyph_cache* glyph;
	bool firstLoop = true;

	fFontCache.reset();

	while ((charCode = UTF8ToCharCode(&string)) > 0) {
		glyph = fFontCache.glyph(charCode);
		if (glyph) {
			if (!firstLoop && fKerning)
				fFontCache.add_kerning(&width, &y);
			width += glyph->advance_x;
		}

		firstLoop = false;
	};

	return width;
}

