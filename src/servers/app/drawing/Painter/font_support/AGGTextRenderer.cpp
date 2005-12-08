// AGGTextRenderer.cpp

#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <FontManager.h>
#include <Message.h>
#include <ServerFont.h>
#include <UTF8.h>

#include <agg_basics.h>
#include <agg_bounding_rect.h>
#include <agg_conv_segmentator.h>
#include <agg_conv_transform.h>
#include <agg_trans_affine.h>

#include "AGGTextRenderer.h"

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


// is_white_space
inline bool
is_white_space(uint16 glyph)
{
	// TODO: handle them all!
	if (glyph == ' ' || glyph == B_TAB)
		return true;
	return false;
}

#define DEFAULT_UNI_CODE_BUFFER_SIZE	2048

// constructor
AGGTextRenderer::AGGTextRenderer()
	: fFontEngine(gFreeTypeLibrary),
	  fFontManager(fFontEngine),

	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves),

	  fUnicodeBuffer((char*)malloc(DEFAULT_UNI_CODE_BUFFER_SIZE)),
	  fUnicodeBufferSize(DEFAULT_UNI_CODE_BUFFER_SIZE),

	  fHinted(true),
	  fAntialias(true),
	  fKerning(true),
	  fEmbeddedTransformation(),

	  fFont(),
	  fLastFamilyAndStyle(0),
	  fLastPointSize(-1.0),
	  fLastHinted(false)
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
	bool success = true;
	// construct an embedded transformation (rotate & shear)
	Transformable transform;
	transform.ShearBy(B_ORIGIN, (90.0 - font.Shear()) * PI / 180.0, 0.0);
	transform.RotateBy(B_ORIGIN, -font.Rotation() * PI / 180.0);

	bool load = font.GetFamilyAndStyle() != fLastFamilyAndStyle ||
				transform != fEmbeddedTransformation;	

	if (load) {
		if (transform.IsIdentity()) {
//printf("AGGTextRenderer::SetFont() - native\n");
			success = fFontEngine.load_font(font, agg::glyph_ren_native_gray8);
		} else {
//printf("AGGTextRenderer::SetFont() - outline\n");
			success = fFontEngine.load_font(font, agg::glyph_ren_outline);
		}
	}

	fLastFamilyAndStyle = font.GetFamilyAndStyle();
	fEmbeddedTransformation = transform;
	fFont = font;

	_UpdateSizeAndHinting(font.Size(), fEmbeddedTransformation.IsIdentity() && fHinted, load);

	if (!success) {
		fprintf(stderr, "font could not be loaded\n");
		return false;
	}

	return true;
}

// SetHinting
void
AGGTextRenderer::SetHinting(bool hinting)
{
	if (fHinted != hinting) {
		fHinted = hinting;
		_UpdateSizeAndHinting(fLastPointSize, fEmbeddedTransformation.IsIdentity() && fHinted);
	}
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

// RenderString
BRect
AGGTextRenderer::RenderString(const char* string,
							  uint32 length,
							  font_renderer_solid_type* solidRenderer,
							  font_renderer_bin_type* binRenderer,
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

	uint32 glyphCount;

	if (_PrepareUnicodeBuffer(string, length, &glyphCount) >= B_OK) {

		fCurves.approximation_scale(transform.scale());
	
		// use a transformation behind the curves
		// (only if glyph->data_type == agg::glyph_data_outline)
		// in the pipeline for the rasterizer
		typedef agg::conv_transform<conv_font_curve_type, agg::trans_affine>	conv_font_trans_type;
		conv_font_trans_type transformedOutline(fCurves, transform);

		uint16* p = (uint16*)fUnicodeBuffer;

		double x  = 0.0;
		double y0 = 0.0;
		double y  = y0;

		double advanceX = 0.0;
		double advanceY = 0.0;

		// for when we bypass the transformation pipeline
		BPoint transformOffset(0.0, 0.0);
		transform.Transform(&transformOffset);

		for (uint32 i = 0; i < glyphCount; i++) {

/*			// line break (not supported by R5)
			if (*p == '\n') {
				y0 += LineOffset();
				x = 0.0;
				y = y0;
				advanceX = 0.0;
				advanceY = 0.0;
				++p;
				continue;
			}*/

			const agg::glyph_cache* glyph = fFontManager.glyph(*p);

			if (glyph) {
				if (i > 0 && fKerning) {
					fFontManager.add_kerning(&advanceX, &advanceY);
				}

				x += advanceX;
				y += advanceY;

				if (delta)
					x += is_white_space(*p) ? delta->space : delta->nonspace;

				// "glyphBounds" is the bounds of the glyph transformed
				// by the x y location of the glyph along the base line,
				// it is therefor yet "untransformed".
				const agg::rect& r = glyph->bounds;
				BRect glyphBounds(r.x1 + x, r.y1 + y - 1, r.x2 + x + 1, r.y2 + y + 1);
					// NOTE: "-1"/"+ 1" converts the glyph bounding box from pixel
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
						fFontManager.init_embedded_adaptors(glyph,
															transformedX,
															transformedY);
						glyphBounds.OffsetBy(transformOffset);
					} else {
						fFontManager.init_embedded_adaptors(glyph, x, y);
						glyphBounds = transform.TransformBounds(glyphBounds);
					}
	
					if (clippingFrame.Intersects(glyphBounds)) {
						switch (glyph->data_type) {
							case agg::glyph_data_mono:
								agg::render_scanlines(fFontManager.mono_adaptor(), 
													  fFontManager.mono_scanline(), 
													  *binRenderer);
								break;
			
							case agg::glyph_data_gray8:
								agg::render_scanlines(fFontManager.gray8_adaptor(), 
													  fFontManager.gray8_scanline(), 
													  *solidRenderer);
								break;
			
							case agg::glyph_data_outline: {
								fRasterizer.reset();
			// NOTE: this function can be easily extended to handle
			// conversion to contours, so that's why there is a lot of
			// commented out code, I leave it here because I think it
			// will be needed again.
			
		//						if(fabs(0.0) <= 0.01) {
								// For the sake of efficiency skip the
								// contour converter if the weight is about zero.
								//-----------------------
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
		/*						} else {
		//							fRasterizer.add_path(fContour);
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
			} else {
				if (*p < 128) {
					char c[2];
					c[0] = (uint8)*p;
					c[1] = 0;
					fprintf(stderr, "failed to load glyph for '%s'\n", c);
				} else {
					fprintf(stderr, "failed to load glyph for %d\n", *p);
				}
			}
			++p;
		}
		// put pen location behind rendered text
		// (at the baseline of the virtual next glyph)
		if (nextCharPos) {
			nextCharPos->x = x + advanceX;
			nextCharPos->y = y + advanceY;

			transform.Transform(nextCharPos);
		}
	}

	return transform.TransformBounds(bounds);
}

// StringWidth
double
AGGTextRenderer::StringWidth(const char* utf8String, uint32 length)
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
	uint32 glyphCount;
	if (_PrepareUnicodeBuffer(utf8String, length, &glyphCount) >= B_OK) {

		uint16* p = (uint16*)fUnicodeBuffer;

		double y  = 0.0;
		const agg::glyph_cache* glyph;

		for (uint32 i = 0; i < glyphCount; i++) {

			if ((glyph = fFontManager.glyph(*p))) {

				if (i > 0 && fKerning) {
					fFontManager.add_kerning(&width, &y);
				}

				width += glyph->advance_x;
			}
			++p;
		}
	}
	return width;
}

// _PrepareUnicodeBuffer
status_t
AGGTextRenderer::_PrepareUnicodeBuffer(const char* utf8String,
									   uint32 length, uint32* glyphCount)
{
	int32 srcLength = length;
	int32 dstLength = srcLength * 4;

	// take care of adjusting buffer size
	if (dstLength > fUnicodeBufferSize) {
		fUnicodeBufferSize = dstLength;
		fUnicodeBuffer = (char*)realloc((void*)fUnicodeBuffer, fUnicodeBufferSize);
	}

	status_t ret;
	if (!fUnicodeBuffer) {
		ret = B_NO_MEMORY;
	} else {
		int32 state = 0;
		ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								utf8String, &srcLength,
								fUnicodeBuffer, &dstLength,
								&state, B_SUBSTITUTE);
	}

	if (ret >= B_OK) {
		*glyphCount = (uint32)(dstLength / 2);
		ret = swap_data(B_INT16_TYPE, fUnicodeBuffer, dstLength,
						B_SWAP_BENDIAN_TO_HOST);
	} else {
		*glyphCount = 0;
		fprintf(stderr, "AGGTextRenderer::_PrepareUnicodeBuffer() - UTF8 -> Unicode conversion failed: %s\n", strerror(ret));
	}

	return ret;
}

// _UpdateSizeAndHinting
void
AGGTextRenderer::_UpdateSizeAndHinting(float size, bool hinted, bool force)
{
	if (force || size != fLastPointSize || hinted != fLastHinted) {
//printf("AGGTextRenderer::_UpdateSizeAndHinting(%.1f, %d, %d)\n", size, hinted, force);
		fLastPointSize = size;
		fLastHinted = hinted;

		fFontEngine.height(size);
		fFontEngine.width(size);
		fFontEngine.hinting(hinted);
	}
}

